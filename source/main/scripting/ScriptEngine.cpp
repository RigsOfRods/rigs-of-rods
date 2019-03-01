/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

/// @file
/// @author Thomas Fischer
/// @date   24th of February 2009

#include "ScriptEngine.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
// AS addons end

#ifdef USE_CURL
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include <cfloat>

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "Collisions.h"
#include "Console.h"
#include "GameContext.h"
#include "GameScript.h"
#include "LocalStorage.h"
#include "OgreScriptBuilder.h"
#include "PlatformUtils.h"
#include "ScriptEvents.h"
#include "VehicleAI.h"

#include "InputEngine.h"

using namespace Ogre;
using namespace RoR;

const char* RoR::ScriptCategoryToString(ScriptCategory c)
{
    switch (c)
    {
    case ScriptCategory::TERRAIN: return "TERRAIN";
    case ScriptCategory::CUSTOM: return "CUSTOM";
    case ScriptCategory::INVALID: return "INVALID";
    default: return "";
    }
}

// some hacky functions

void logString(const std::string &str)
{
    App::GetScriptEngine()->SLOG(str);
}

// the class implementation

ScriptEngine::ScriptEngine() :
     context(0)
    , engine(0)
    , scriptLog(0)
{
    scriptLog = LogManager::getSingleton().createLog(PathCombine(App::sys_logs_dir->getStr(), "Angelscript.log"), false);
    this->init();
}

ScriptEngine::~ScriptEngine()
{
    // Clean up
    if (engine)  engine->Release();
    if (context) context->Release();
}

void ScriptEngine::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName, bool& skipThisMessage)
{
    RoR::App::GetConsole()->forwardLogMessage(Console::CONSOLE_MSGTYPE_SCRIPT, message, lml);
}

// continue with initializing everything
void ScriptEngine::init()
{
    SLOG("ScriptEngine initializing ...");
    int result;

    // Create the script engine
    engine = AngelScript::asCreateScriptEngine(ANGELSCRIPT_VERSION);

    engine->SetEngineProperty(AngelScript::asEP_ALLOW_UNSAFE_REFERENCES, true); // Needed for ImGui

    // Set the message callback to receive information on errors in human readable form.
    // It's recommended to do this right after the creation of the engine, because if
    // some registration fails the engine may send valuable information to the message
    // stream.
    result = engine->SetMessageCallback(AngelScript::asMETHOD(ScriptEngine,msgCallback), this, AngelScript::asCALL_THISCALL);
    if (result < 0)
    {
        if (result == AngelScript::asINVALID_ARG)
        {
            SLOG("One of the arguments is incorrect, e.g. obj is null for a class method.");
            return;
        }
        else if (result == AngelScript::asNOT_SUPPORTED)
        {
            SLOG("	The arguments are not supported, e.g. asCALL_GENERIC.");
            return;
        }
        SLOG("Unkown error while setting up message callback");
        return;
    }

    // AngelScript doesn't have a built-in string type, as there is no definite standard
    // string type for C++ applications. Every developer is free to register it's own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to register your own string type if you don't want to.
    AngelScript::RegisterScriptArray(engine, true);
    AngelScript::RegisterStdString(engine);
    AngelScript::RegisterStdStringUtils(engine);
    AngelScript::RegisterScriptMath(engine);
    static float SCRIPT_FLT_MAX = FLT_MAX;
    static int SCRIPT_INT_MAX = INT_MAX;
    result = engine->RegisterGlobalProperty("const float FLT_MAX", &SCRIPT_FLT_MAX); ROR_ASSERT( result >= 0 );
    result = engine->RegisterGlobalProperty("const int INT_MAX", &SCRIPT_INT_MAX); ROR_ASSERT(result >= 0);
    AngelScript::RegisterScriptAny(engine);
    AngelScript::RegisterScriptDictionary(engine);

    // some useful global functions
    result = engine->RegisterGlobalFunction("void log(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); ROR_ASSERT( result >= 0 );
    result = engine->RegisterGlobalFunction("void print(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); ROR_ASSERT( result >= 0 );

    RegisterOgreObjects(engine);   // vector2/3, degree, radian, quaternion, color
    RegisterLocalStorage(engine);  // LocalStorage
    RegisterInputEngine(engine);   // InputEngineClass, inputEvents
    RegisterImGuiBindings(engine); // ImGUi::
    RegisterVehicleAi(engine);     // VehicleAIClass, aiEvents, AiValues
    RegisterConsole(engine);       // ConsoleClass, CVarClass, CVarFlags
    RegisterActor(engine);         // BeamClass
    RegisterProceduralRoad(engine);// procedural_point, ProceduralRoadClass, ProceduralObjectClass, ProceduralManagerClass
    RegisterTerrain(engine);       // TerrainClass
    RegisterGameScript(engine);    // GameScriptClass
    RegisterScriptEvents(engine);  // scriptEvents
    RegisterGenericFileFormat(engine); // TokenType, GenericDocumentClass, GenericDocReaderClass

    // now the global instances
    result = engine->RegisterGlobalProperty("GameScriptClass game", &m_game_script); ROR_ASSERT(result>=0);
    result = engine->RegisterGlobalProperty("ConsoleClass console", App::GetConsole()); ROR_ASSERT(result>=0);
    result = engine->RegisterGlobalProperty("InputEngineClass inputs", App::GetInputEngine()); ROR_ASSERT(result>=0);

    SLOG("Type registrations done. If you see no error above everything should be working");

    context = engine->CreateContext();
}

void ScriptEngine::msgCallback(const AngelScript::asSMessageInfo *msg)
{
    const char *type = "Error";
    if ( msg->type == AngelScript::asMSGTYPE_INFORMATION )
        type = "Info";
    else if ( msg->type == AngelScript::asMSGTYPE_WARNING )
        type = "Warning";

    char tmp[1024]="";
    sprintf(tmp, "%s (%d, %d): %s = %s", msg->section, msg->row, msg->col, type, msg->message);
    SLOG(tmp);
}

int ScriptEngine::framestep(Real dt)
{
    // Check if we need to execute any strings
    std::vector<String> tmpQueue;
    stringExecutionQueue.pull(tmpQueue);
    std::vector<String>::iterator it;
    for (it=tmpQueue.begin(); it!=tmpQueue.end();it++)
    {
        executeString(*it);
    }

    // framestep stuff below
    if (!engine || !context) return 0;

    for (auto& pair: m_script_units)
    {
        ScriptUnitId_t id = pair.first;
        if (!m_script_units[id].frameStepFunctionPtr)
        {
            continue;
        }

        context->Prepare(m_script_units[id].frameStepFunctionPtr);

        // Set the function arguments
        context->SetArgFloat(0, dt);

        m_currently_executing_script_unit = id;
        int r = context->Execute();
        m_currently_executing_script_unit = SCRIPTUNITID_INVALID;
        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
            // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }
    return 0;
}

int ScriptEngine::fireEvent(std::string instanceName, float intensity)
{
    if (!engine || !context)
        return 0;

    for (auto& pair: m_script_units)
    {
        ScriptUnitId_t id = pair.first;
        AngelScript::asIScriptFunction* func = m_script_units[id].scriptModule->GetFunctionByDecl(
            "void fireEvent(string, float)"); // TODO: this shouldn't be hard coded --neorej16

        context->Prepare(func);

        // Set the function arguments
        context->SetArgObject(0, &instanceName);
        context->SetArgFloat (1, intensity);

        m_currently_executing_script_unit = id;
        int r = context->Execute();
        m_currently_executing_script_unit = SCRIPTUNITID_INVALID;
        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
          // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }

    return 0;
}

void ScriptEngine::envokeCallback(int _functionId, eventsource_t *source, node_t *node, int type)
{
    if (!engine || !context)
        return;

    for (auto& pair: m_script_units)
    {
        ScriptUnitId_t id = pair.first;
        int functionId = _functionId;
        if (functionId <= 0 && (m_script_units[id].defaultEventCallbackFunctionPtr != nullptr))
        {
            // use the default event handler instead then
            functionId = m_script_units[id].defaultEventCallbackFunctionPtr->GetId();
        }
        else if (functionId <= 0)
        {
            // no default callback available, discard the event
            return;
        }

        context->Prepare(engine->GetFunctionById(functionId));

        // Set the function arguments
        context->SetArgDWord (0, type);
        context->SetArgObject(1, &source->es_instance_name);
        context->SetArgObject(2, &source->es_box_name);
        if (node)
            context->SetArgDWord (3, static_cast<AngelScript::asDWORD>(node->pos));
        else
            context->SetArgDWord (3, static_cast<AngelScript::asDWORD>(-1));

        m_currently_executing_script_unit = id;
        int r = context->Execute();
        m_currently_executing_script_unit = SCRIPTUNITID_INVALID;

        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
            // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }
}

void ScriptEngine::queueStringForExecution(const String command)
{
    stringExecutionQueue.push(command);
}

int ScriptEngine::executeString(String command)
{
    if (!engine || !context)
        return 1;

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;
    int result = ExecuteString(engine, command.c_str(), mod, context);
    if (result < 0)
    {
        SLOG("error " + TOSTRING(result) + " while executing string: " + command + ".");
    }
    return result;
}

int ScriptEngine::addFunction(const String &arg)
{
    if (!engine || !context)
        return 1;

    if (!context)
        

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    AngelScript::asIScriptFunction *func = 0;
    int r = mod->CompileFunction("addfunc", arg.c_str(), 0, AngelScript::asCOMP_ADD_TO_MODULE, &func);
    
    if ( r < 0 )
    {
        char tmp[512] = "";
        snprintf(tmp, 512, "An error occurred while trying to add a function ('%s') to script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }
    else
    {
        // successfully added function; Check if we added a "special" function

        if (func == mod->GetFunctionByDecl("void frameStep(float)"))
        {
            if (m_script_units[m_terrain_script_unit].frameStepFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].frameStepFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void eventCallback(int, int)"))
        {
            if (m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)"))
        {
            if (m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr = func;
        }
    }

    // We must release the function object
    if ( func )
        func->Release();

    return r;
}

int ScriptEngine::functionExists(const String &arg)
{
    if (!engine || !context) // WTF? If the scripting engine failed to start, how would it invoke this function?
        return -1; // ... OK, I guess the author wanted the fn. to be usable both within script and C++, but IMO that's bad design (generally good, but bad for a game.. bad for RoR), really ~ only_a_ptr, 09/2017

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return -1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    if (mod == 0)
    {
        return AngelScript::asNO_FUNCTION; // Nope, it's an internal error, not a "function not found" case ~ only_a_ptr, 09/2017
    }
    else
    {
        AngelScript::asIScriptFunction* fn = mod->GetFunctionByDecl(arg.c_str());
        if (fn != nullptr)
            return fn->GetId();
        else
            return AngelScript::asNO_FUNCTION;
    }
}

int ScriptEngine::deleteFunction(const String &arg)
{
    if (!engine || !context)
        return AngelScript::asERROR;

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return -1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    if ( mod->GetFunctionCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s': No functions have been added (and consequently: the function does not exist).", arg.c_str(), mod->GetName());
        SLOG(tmp);
        return AngelScript::asNO_FUNCTION;
    }

    AngelScript::asIScriptFunction* func = mod->GetFunctionByDecl(arg.c_str());
    if (func != nullptr)
    {
        // Warning: The function is not destroyed immediately, only when no more references point to it.
        mod->RemoveFunction(func);

        // Since functions can be recursive, we'll call the garbage
        // collector to make sure the object is really freed
        engine->GarbageCollect();

        // Check if we removed a "special" function

        if ( m_script_units[m_terrain_script_unit].frameStepFunctionPtr == func )
            m_script_units[m_terrain_script_unit].frameStepFunctionPtr = nullptr;

        if ( m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr == func )
            m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr = nullptr;

        if ( m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr == func )
            m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr = nullptr;

        return func->GetId();
    }
    else
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
        return AngelScript::asERROR;
    }
}

int ScriptEngine::addVariable(const String &arg)
{
    if (!engine || !context) return 1;
    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    int r = mod->CompileGlobalVar("addvar", arg.c_str(), 0);
    if ( r < 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to add a variable ('%s') to script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }

    return r;
}

int ScriptEngine::deleteVariable(const String &arg)
{
    if (!engine || !context) return 1;
    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit == SCRIPTUNITID_INVALID)
        return 1;
    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    if ( mod == 0 || mod->GetGlobalVarCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s': No variables have been added (and consequently: the variable does not exist).", arg.c_str(), mod->GetName());
        SLOG(tmp);
        return AngelScript::asNO_GLOBAL_VAR;
    }

    int index = mod->GetGlobalVarIndexByName(arg.c_str());
    if ( index >= 0 )
    {
        index = mod->RemoveGlobalVar(index);
    }
    else
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }

    return index;
}

void ScriptEngine::triggerEvent(int eventnum, int value)
{
    if (!engine || !context) return;

    for (auto& pair: m_script_units)
    {
        ScriptUnitId_t id = pair.first;
        if (m_script_units[id].eventCallbackFunctionPtr==nullptr)
            continue;
        if (m_script_units[id].eventMask & eventnum)
        {
            // script registered for that event, so sent it
            context->Prepare(m_script_units[id].eventCallbackFunctionPtr);

            // Set the function arguments
            context->SetArgDWord(0, eventnum);
            context->SetArgDWord(1, value);

            m_currently_executing_script_unit = id;
            int r = context->Execute();
            m_currently_executing_script_unit = SCRIPTUNITID_INVALID;
            if ( r == AngelScript::asEXECUTION_FINISHED )
            {
                // The return value is only valid if the execution finished successfully
                AngelScript::asDWORD ret = context->GetReturnDWord();
            }
        }
    }
}

String ScriptEngine::composeModuleName(String const& scriptName, ScriptCategory origin, ScriptUnitId_t id)
{
    return fmt::format("{}(category:{},unique ID:{})", scriptName, ScriptCategoryToString(origin), id);
}

ScriptUnitId_t ScriptEngine::loadScript(String scriptName, ScriptCategory category/* = ScriptCategory::TERRAIN*/)
{
    // This function creates a new script unit, tries to set it up and removes it if setup fails.
    // -----------------------------------------------------------------------------------------
    // A script unit is how Rigs of Rods organizes scripts from various sources.
    // Because the script is executed during loading, it's wrapping unit must
    // be created early, and removed if setup fails.
    static ScriptUnitId_t id_counter = 0;

    ScriptUnitId_t unit_id = id_counter++;
    auto itor_pair = m_script_units.insert(std::make_pair(unit_id, ScriptUnit()));
    m_script_units[unit_id].uniqueId = unit_id;
    m_script_units[unit_id].scriptName = scriptName;
    m_script_units[unit_id].scriptCategory = category;
    if (category == ScriptCategory::TERRAIN)
    {
        m_terrain_script_unit = unit_id;
    }    

    // Perform the actual script loading, building and running main().
    int result = this->setupScriptUnit(unit_id);

    // If setup failed, remove the unit.
    if (result != 0)
    {
        m_script_units.erase(itor_pair.first);
        if (category == ScriptCategory::TERRAIN)
        {
            m_terrain_script_unit = SCRIPTUNITID_INVALID;
        }
        return SCRIPTUNITID_INVALID;
    }

    return unit_id;
}

int ScriptEngine::setupScriptUnit(int unit_id)
{
    int result=0;

    String moduleName = this->composeModuleName(
        m_script_units[unit_id].scriptName, m_script_units[unit_id].scriptCategory, m_script_units[unit_id].uniqueId);

    // The builder is a helper class that will load the script file,
    // search for #include directives, and load any included files as
    // well.
    OgreScriptBuilder builder;

    // A script module is how AngelScript organizes scripts.
    // It contains the script loaded by user plus all `#include`-d scripts.
    result = builder.StartNewModule(engine, moduleName.c_str());
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to create module.", moduleName));
        return result;
    }
    m_script_units[unit_id].scriptModule = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);

    // Load the script from the file system.
    result = builder.AddSectionFromFile(m_script_units[unit_id].scriptName.c_str());
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to process file.", moduleName));
        return result;
    }

    // Build the AngelScript module - this loads `#include`-d scripts
    // and runs any global statements, for example constructors of
    // global objects like raceManager in 'races.as'. For this reason,
    // the game must already be aware of the script, but only temporarily.
    m_currently_executing_script_unit = unit_id; // for `BuildModule()` below.
    result = builder.BuildModule();
    m_currently_executing_script_unit = SCRIPTUNITID_INVALID; // Tidy up.
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to build module. See 'Angelscript.log' for more info.", moduleName));
        return result;
    }

    String scriptHash;
    if (m_script_units[unit_id].scriptCategory == ScriptCategory::TERRAIN) // Classic behavior
        scriptHash = builder.GetHash();

    // get some other optional functions
    m_script_units[unit_id].frameStepFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void frameStep(float)");

    m_script_units[unit_id].eventCallbackFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void eventCallback(int, int)");

    m_script_units[unit_id].defaultEventCallbackFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)");

    // Find the function that is to be called.
    auto main_func = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void main()");
    if ( main_func == nullptr )
    {
        // The function couldn't be found. Instruct the script writer to include the
        // expected function in the script.
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - there is no function `main()`", moduleName));
        return 0;
    }

    // Prepare the script context with the function we wish to execute. Prepare()
    // must be called on the context before each new script function that will be
    // executed. Note, that if you intend to execute the same function several
    // times, it might be a good idea to store the function id returned by
    // GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
    result = context->Prepare(main_func);
    if (result < 0)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to build module.", moduleName));
        context->Release();
        return -1;
    }

    // Execute the `main()` function in the script.
    // The function must have full access to the game API.
    SLOG(fmt::format("Executing main() in {}", moduleName));
    m_currently_executing_script_unit = unit_id; // for `Execute()` below.
    result = context->Execute();
    m_currently_executing_script_unit = SCRIPTUNITID_INVALID; // Tidy up.
    if ( result != AngelScript::asEXECUTION_FINISHED )
    {
        // The execution didn't complete as expected. Determine what happened.
        if ( result == AngelScript::asEXECUTION_ABORTED )
        {
            SLOG("The script was aborted before it could finish. Probably it timed out.");
        }
        else if ( result == AngelScript::asEXECUTION_EXCEPTION )
        {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            SLOG("An exception '" + String(context->GetExceptionString()) 
            + "' occurred. Please correct the code in file '" 
            + m_script_units[unit_id].scriptName + "' and try again.");

            // Write some information about the script exception
            AngelScript::asIScriptFunction* func = context->GetExceptionFunction();
            SLOG("func: " + String(func->GetDeclaration()));
            SLOG("modl: " + String(func->GetModuleName()));
            SLOG("sect: " + String(func->GetScriptSectionName()));
            SLOG("line: " + TOSTRING(context->GetExceptionLineNumber()));
            SLOG("desc: " + String(context->GetExceptionString()));
        }
        else
        {
            SLOG("The script ended for some unforeseen reason " + TOSTRING(result));
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - error running function `main()`, check AngelScript.log", moduleName));
    }
    else
    {
        SLOG("The script finished successfully.");
    }

    return 0;
}

void ScriptEngine::unloadScript(ScriptUnitId_t id)
{
    ROR_ASSERT(id != SCRIPTUNITID_INVALID);
    ROR_ASSERT(m_currently_executing_script_unit == SCRIPTUNITID_INVALID);

    engine->DiscardModule(m_script_units[id].scriptModule->GetName());
    m_script_units.erase(id);
    if (m_terrain_script_unit == id)
    {
        m_terrain_script_unit = SCRIPTUNITID_INVALID;
    }
}

void ScriptEngine::activateLogging()
{
    scriptLog->addListener(this);
}

ScriptUnit& ScriptEngine::getScriptUnit(ScriptUnitId_t unique_id)
{
    ROR_ASSERT(unique_id != SCRIPTUNITID_INVALID);
    ROR_ASSERT(m_script_units.count(unique_id) != 0);
    return m_script_units[unique_id];
}
