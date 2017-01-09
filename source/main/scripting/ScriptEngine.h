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

#pragma once

#ifdef USE_ANGELSCRIPT

#define DEFAULT_TERRAIN_SCRIPT "default.as" // Used when map creator doesn't provide custom script.

#include "AngelScriptBindings.h"
#include "Application.h"
#include "GameContext.h"
#include "GameScript.h"
#include "InterThreadStoreVector.h"
#include "ScriptEvents.h"

#include <Ogre.h>
#include "scriptdictionary/scriptdictionary.h"
#include "scriptbuilder/scriptbuilder.h"

#include <map>

namespace RoR {

/// @addtogroup Scripting
/// @{

/// Asynchronously (via `MSG_SIM_SCRIPT_EVENT_TRIGGERED`) invoke script function `eventCallbackEx()`, if registered, otherwise fall back to `eventCallback()`
inline void TRIGGER_EVENT_ASYNC(scriptEvents type, int arg1, int arg2ex = 0, int arg3ex = 0, int arg4ex = 0, std::string arg5ex = "", std::string arg6ex = "", std::string arg7ex = "", std::string arg8ex = "")
{
    ScriptEventArgs* args = new ScriptEventArgs{ type, arg1, arg2ex, arg3ex, arg4ex, arg5ex, arg6ex, arg7ex, arg8ex };
    App::GetGameContext()->PushMessage(Message(MSG_SIM_SCRIPT_EVENT_TRIGGERED, (void*)args));
}

/// Note: Either of these can be loaded from script using `game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED...)`
enum class ScriptCategory
{
    INVALID,
    ACTOR,   //!< Defined in truck file under 'scripts', contains global variable `BeamClass@ thisActor`.
    TERRAIN, //!< Defined in terrn2 file under '[Scripts]', receives terrain eventbox notifications.
    GADGET,  //!< Associated with a .gadget mod file, launched via UI or any method given below for CUSTOM scripts (use .gadget suffix - game will fix up category to `GADGET`).
    CUSTOM   //!< Loaded by user via either: A) ingame console 'loadscript'; B) RoR.cfg 'app_custom_scripts'; C) commandline '-runscript'; If used with .gadget file, game will fix up category to `GADGET`.
};

const char* ScriptCategoryToString(ScriptCategory c);

/// Represents a loaded script and all associated resources/handles.
struct ScriptUnit
{
    ScriptUnit();
    ~ScriptUnit();

    ScriptUnitID_t uniqueId = SCRIPTUNITID_INVALID;
    ScriptCategory scriptCategory = ScriptCategory::INVALID;
    unsigned int eventMask = 0; //!< filter mask for script events
    AngelScript::asIScriptModule* scriptModule = nullptr;
    AngelScript::asIScriptFunction* frameStepFunctionPtr = nullptr; //!< script function pointer to the frameStep function
    AngelScript::asIScriptFunction* eventCallbackFunctionPtr = nullptr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* eventCallbackExFunctionPtr = nullptr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* defaultEventCallbackFunctionPtr = nullptr; //!< script function pointer for spawner events
    ActorPtr associatedActor; //!< For ScriptCategory::ACTOR
    CacheEntryPtr originatingGadget; //!< For ScriptCategory::GADGET ~ determines resource group
    Ogre::String scriptName; //!< Name of the '.as' file exclusively.
    Ogre::String scriptHash;
    Ogre::String scriptBuffer;
};

typedef std::map<ScriptUnitID_t, ScriptUnit> ScriptUnitMap;

struct LoadScriptRequest
{
    std::string lsr_filename; //!< Load from resource ('.as' file or '.gadget' file); If buffer is supplied, use this as display name only.
    std::string lsr_buffer; //!< Load from memory buffer.
    ScriptCategory lsr_category = ScriptCategory::TERRAIN;
    ActorInstanceID_t lsr_associated_actor = ACTORINSTANCEID_INVALID; //!< For ScriptCategory::ACTOR
};

struct ScriptCallbackArgs
{
    ScriptCallbackArgs(eventsource_t* evs, NodeNum_t nd): eventsource(evs), node(nd) {}

    eventsource_t* eventsource = nullptr;
    NodeNum_t node = NODENUM_INVALID;
};

typedef BitMask_t GetFuncFlags_t; //!< Flags for `RoR::ScriptEngine::getFunctionByDeclAndLogCandidates()`
const GetFuncFlags_t GETFUNCFLAG_OPTIONAL = 0; //!< Only logs warning if candidate is found, to help modder find a typo.
const GetFuncFlags_t GETFUNCFLAG_REQUIRED = BITMASK(1); //!< Always logs warning that function was not found.
const GetFuncFlags_t GETFUNCFLAG_SILENT = BITMASK(2); //!< Never logs

// Params to `RoR::ScriptEngine::getFunctionByDeclAndLogCandidates()`
const std::string GETFUNC_DEFAULTEVENTCALLBACK_SIGFMT = "void {}(int, string, string, int)";
const std::string GETFUNC_DEFAULTEVENTCALLBACK_NAME = "defaultEventCallback";

/// Common return codes for script manipulation funcs (add/get/delete | funcs/variables)
enum ScriptRetCode
{
    // Generic success - 0 by common convention.
    SCRIPTRETCODE_SUCCESS = AngelScript::asSUCCESS, // = AngelScript::0

    // AngelScript technical codes
    SCRIPTRETCODE_AS_ERROR = AngelScript::asERROR, // = AngelScript::-1 etc...
    SCRIPTRETCODE_AS_CONTEXT_ACTIVE = AngelScript::asCONTEXT_ACTIVE,
    SCRIPTRETCODE_AS_CONTEXT_NOT_FINISHED = AngelScript::asCONTEXT_NOT_FINISHED,
    SCRIPTRETCODE_AS_CONTEXT_NOT_PREPARED = AngelScript::asCONTEXT_NOT_PREPARED,
    SCRIPTRETCODE_AS_INVALID_ARG = AngelScript::asINVALID_ARG,
    SCRIPTRETCODE_AS_NO_FUNCTION = AngelScript::asNO_FUNCTION,
    SCRIPTRETCODE_AS_NOT_SUPPORTED = AngelScript::asNOT_SUPPORTED,
    SCRIPTRETCODE_AS_INVALID_NAME = AngelScript::asINVALID_NAME,
    SCRIPTRETCODE_AS_NAME_TAKEN = AngelScript::asNAME_TAKEN,
    SCRIPTRETCODE_AS_INVALID_DECLARATION = AngelScript::asINVALID_DECLARATION,
    SCRIPTRETCODE_AS_INVALID_OBJECT = AngelScript::asINVALID_OBJECT,
    SCRIPTRETCODE_AS_INVALID_TYPE = AngelScript::asINVALID_TYPE,
    SCRIPTRETCODE_AS_ALREADY_REGISTERED = AngelScript::asALREADY_REGISTERED,
    SCRIPTRETCODE_AS_MULTIPLE_FUNCTIONS = AngelScript::asMULTIPLE_FUNCTIONS,
    SCRIPTRETCODE_AS_NO_MODULE = AngelScript::asNO_MODULE,
    SCRIPTRETCODE_AS_NO_GLOBAL_VAR = AngelScript::asNO_GLOBAL_VAR,
    SCRIPTRETCODE_AS_INVALID_CONFIGURATION = AngelScript::asINVALID_CONFIGURATION,
    SCRIPTRETCODE_AS_INVALID_INTERFACE = AngelScript::asINVALID_INTERFACE,
    SCRIPTRETCODE_AS_CANT_BIND_ALL_FUNCTIONS = AngelScript::asCANT_BIND_ALL_FUNCTIONS,
    SCRIPTRETCODE_AS_LOWER_ARRAY_DIMENSION_NOT_REGISTERED = AngelScript::asLOWER_ARRAY_DIMENSION_NOT_REGISTERED,
    SCRIPTRETCODE_AS_WRONG_CONFIG_GROUP = AngelScript::asWRONG_CONFIG_GROUP,
    SCRIPTRETCODE_AS_CONFIG_GROUP_IS_IN_USE = AngelScript::asCONFIG_GROUP_IS_IN_USE,
    SCRIPTRETCODE_AS_ILLEGAL_BEHAVIOUR_FOR_TYPE = AngelScript::asILLEGAL_BEHAVIOUR_FOR_TYPE,
    SCRIPTRETCODE_AS_WRONG_CALLING_CONV = AngelScript::asWRONG_CALLING_CONV,
    SCRIPTRETCODE_AS_BUILD_IN_PROGRESS = AngelScript::asBUILD_IN_PROGRESS,
    SCRIPTRETCODE_AS_INIT_GLOBAL_VARS_FAILED = AngelScript::asINIT_GLOBAL_VARS_FAILED,
    SCRIPTRETCODE_AS_OUT_OF_MEMORY = AngelScript::asOUT_OF_MEMORY,
    SCRIPTRETCODE_AS_MODULE_IS_IN_USE = AngelScript::asMODULE_IS_IN_USE,

    // RoR ScriptEngine return codes
    //      Following is analysis of former state (most but not all funcs returned 0 on success)
    //       ~ executeString() [script: not exported] - used to return 0 on success, 1 on internal error and negative number otherwise.
    //       ~ addFunction() [script: `game.addScriptFunction()`] - used to return 0 on success, 1 on internal error and negative number otherwise.
    //       ~ functionExists() [script: `game.scriptFunctionExists()`] - used to return function ID (always >0) on success, negative number on error.
    //       ~ deleteFunction() [script: `game.deleteScriptFunction()`] - used to return function ID (always >0) on success, negative number on error.
    //       ~ addVariable() [script: `game.addScriptVariable()` ] - used to return 0 on success, 1 on internal error and negative number otherwise.
    //       ~ variableExists() [script: `game.scriptVariableExists()`] - newly added, returns 0 on success  and negative number otherwise.
    //       ~ deleteVariable() [script: `game.deleteScriptVariable()` ] - used to return 0 on success, 1 on internal error and negative number otherwise.
    //       ~ getVariable() [script: `game.getScriptVariable()` ] - recently added, returns 0 on success  and negative number otherwise.
    SCRIPTRETCODE_UNSPECIFIED_ERROR = -1001,
    SCRIPTRETCODE_ENGINE_NOT_CREATED = -1002,
    SCRIPTRETCODE_CONTEXT_NOT_CREATED = -1003,
    SCRIPTRETCODE_SCRIPTUNIT_NOT_EXISTS = -1004, // The scpecified ScriptUnitID_t was invalid
    SCRIPTRETCODE_SCRIPTUNIT_NO_MODULE = -1005,
    SCRIPTRETCODE_FUNCTION_NOT_EXISTS = -1006,
};

/**
 *  @brief This class represents the angelscript scripting interface. It can load and execute scripts.
 *  @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)
 */
class ScriptEngine : public Ogre::LogListener
{
    friend class GameScript;

public:

    ScriptEngine();
    ~ScriptEngine();

    /**
     * Loads a script
     * @param filename '.as' file or '.gadget' file to load; if buffer is supplied, this is only a display name.
     * @param category How to treat the script?
     * @param associatedActor Only for category ACTOR
     * @param buffer String with full script body; if empty, a file will be loaded as usual.
     * @return Unique ID of the script unit (because one script file can be loaded multiple times).
     */
    ScriptUnitID_t loadScript(Ogre::String filename, ScriptCategory category = ScriptCategory::TERRAIN,
        ActorPtr associatedActor = nullptr, std::string buffer = "");

    /**
     * Unloads a script
     * @param unique_id The script unit ID as returned by `loadScript()`
     */
    void unloadScript(ScriptUnitID_t unique_id);

    /**
     * Calls the script's framestep function to be able to use timed things inside the script
     * @param dt time passed since the last call to this function in seconds
     */
    void framestep(Ogre::Real dt);

    void setForwardScriptLogToConsole(bool doForward);

    /**
     * triggers an event; Not to be used by the end-user.
     * Runs either `eventCallbackEx()`, if registered, or `eventCallback()`, if registered, in this order.
     */
    void triggerEvent(scriptEvents eventnum, int arg1=0, int arg2ex=0, int arg3ex=0, int arg4ex=0, std::string arg5ex="", std::string arg6ex="", std::string arg7ex="", std::string arg8ex="");

    void setEventsEnabled(bool val) { m_events_enabled = val; }

    /**
     * executes a string (useful for the console)
     * @param command string to execute
     */
    ScriptRetCode_t executeString(Ogre::String command);

    /**
     * Queues a string for execution.
     * Use this when you want to execute a script statement from another thread.
     * @param command string to queue for execution
     * @see executeString
     */
    void queueStringForExecution(const Ogre::String command);

    /**
     * Adds a global function to the script
     * @param arg A declaration for the function.
     * @param nid The script unit ID to act upon - by default the terrain script.
    */
    ScriptRetCode_t addFunction(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
     * Checks if a global function exists
     * @param arg A declaration for the function.
    */
    ScriptRetCode_t functionExists(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
     * Deletes a global function from the script
     * @param arg A declaration for the function.
    */
    ScriptRetCode_t deleteFunction(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
     * Adds a global variable to the script
     * @param arg A declaration for the variable.
    */
    ScriptRetCode_t addVariable(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
     * Adds a global variable to the script
     * @param arg A declaration for the variable.
    */
    ScriptRetCode_t variableExists(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
     * Deletes a global variable from the script
     * @param arg A declaration for the variable.
    */
    ScriptRetCode_t deleteVariable(const Ogre::String& arg, const ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
    * Retrieves a global variable from any running script
    */
    ScriptRetCode_t getVariable(const Ogre::String& varName, void *ref, int typeID, ScriptUnitID_t nid = SCRIPTUNITID_DEFAULT);

    /**
    * Finds a function by full declaration, and if not found, finds candidates by name and logs them to Angelscript.log
    * @return Angelscript function on success, null on error.
    */
    AngelScript::asIScriptFunction* getFunctionByDeclAndLogCandidates(ScriptUnitID_t nid, GetFuncFlags_t flags, const std::string& funcName, const std::string& fmtFuncDecl);

    int fireEvent(std::string instanceName, float intensity);

    void envokeCallback(int functionId, eventsource_t* source, NodeNum_t nodenum = NODENUM_INVALID, int type = 0, int truckNum=-1); //cosmic vole added truckNum to ease detection of AI trucks at checkpoints etc.

    /**
    * Forwards useful info from C++ `try{}catch{}` exceptions to script in the form of game event.
    * AngelScript doesn't have exceptions in this sense (in AS jargon, 'Exception' means basically 'panic' as in Lua/Rust...)
    * and most exceptions this game encounters (`Ogre::Exception`) are trivially recoverable, so it doesn't make sense
    * to panic AngelScript when they happen.
    */
    void forwardExceptionAsScriptEvent(const std::string& from);

    AngelScript::asIScriptEngine* getEngine() { return engine; };

    // method from Ogre::LogListener
    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    inline void SLOG(const char* msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro
    inline void SLOG(std::string msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro

    bool scriptUnitExists(ScriptUnitID_t unique_id);
    ScriptUnit& getScriptUnit(ScriptUnitID_t unique_id);
    ScriptUnitID_t getTerrainScriptUnit() const { return m_terrain_script_unit; } //!< @return SCRIPTUNITID_INVALID if none exists.
    ScriptUnitID_t getCurrentlyExecutingScriptUnit() const { return m_currently_executing_script_unit; } //!< @return SCRIPTUNITID_INVALID if none is executing right now.
    ScriptUnitMap const& getScriptUnits() const { return m_script_units; }

protected:

    /// @name Housekeeping
    /// @{

    /**
     * This function initialzies the engine and registeres all types
     */
    void init();

    /**
    * Packs name + important info to one string, for logging and reporting purposes.
    */
    Ogre::String composeModuleName(Ogre::String const& scriptName, ScriptCategory origin, ScriptUnitID_t id);

    /**
    * Helper for `loadScript()`, does the actual building without worry about unit management.
    * @return 0 on success, anything else on error.
    */
    int setupScriptUnit(int unit_id);

    /**
    * Helper for executing any script function/snippet; does `asIScriptContext::Prepare()` and reports any error.
    * @return true on success, false on error.
    */
    bool prepareContextAndHandleErrors(ScriptUnitID_t nid, int asFunctionID);

    /**
    * Helper for executing any script function/snippet; registers Line/Exception callbacks (on demand) and set currently executed NID; The `asIScriptContext::Prepare()` and setting args must be already done.
    * @return 0 on success, anything else on error.
    */
    int executeContextAndHandleErrors(ScriptUnitID_t nid);

    /**
    * Helper for all manipulations with functions/variables; ensures the script unit exists and is fully set up.
    * @return see `RoR::ScriptRetCode` ~ 0 on success, negative number on error.
    */
    ScriptRetCode_t validateScriptModule(const ScriptUnitID_t nid, AngelScript::asIScriptModule*& out_mod);

    /// @}

    /// @name Script diagnostics
    /// @{

    /**
     * Optional (but very recommended!) callback providing diagnostic info when things fail to start (most notably script errors).
     */
    void msgCallback(const AngelScript::asSMessageInfo* msg);

    /**
    * Optional callback which receives diagnostic info for every executed statement.
    * https://www.angelcode.com/angelscript/sdk/docs/manual/classas_i_script_context.html#ae2747f643bf9a07364f922c460ef57dd
    */
    void lineCallback(AngelScript::asIScriptContext* ctx);

    /**
    * Optional callback invoked when the script critically fails, allowing debugging.
    * https://www.angelcode.com/angelscript/sdk/docs/manual/doc_call_script_func.html#doc_call_script_4
    */
    void exceptionCallback(AngelScript::asIScriptContext* ctx);

    /// @}

    AngelScript::asIScriptEngine* engine; //!< instance of the scripting engine
    AngelScript::asIScriptContext* context; //!< context in which all scripting happens
    Ogre::Log*      scriptLog;
    GameScript      m_game_script;
    ScriptUnitMap   m_script_units;
    ScriptUnitID_t  m_terrain_script_unit = SCRIPTUNITID_INVALID;
    ScriptUnitID_t  m_currently_executing_script_unit = SCRIPTUNITID_INVALID;
    scriptEvents    m_currently_executing_event_trigger = SE_NO_EVENTS;
    bool            m_events_enabled = true; //!< Hack to enable fast shutdown without cleanup

    InterThreadStoreVector<Ogre::String> stringExecutionQueue; //!< The string execution queue \see queueStringForExecution
};

/// @}   //addtogroup Scripting

} // namespace RoR

#else // USE_ANGELSCRIPT
inline void TRIGGER_EVENT_ASYNC(scriptEvents type, int arg1, int arg2ex = 0, int arg3ex = 0, int arg4ex = 0, std::string arg5ex = "", std::string arg6ex = "", std::string arg7ex = "", std::string arg8ex = "")
{
}
#endif // USE_ANGELSCRIPT
