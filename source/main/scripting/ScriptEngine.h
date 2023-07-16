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
    CUSTOM   //!< Loaded by user via either: A) ingame console 'loadscript'; B) RoR.cfg 'app_custom_scripts'; C) commandline '-runscript'.
};

const char* ScriptCategoryToString(ScriptCategory c);

/// Represents a loaded script and all associated resources/handles.
struct ScriptUnit
{
    ScriptUnitId_t uniqueId = SCRIPTUNITID_INVALID;
    ScriptCategory scriptCategory = ScriptCategory::INVALID;
    unsigned int eventMask = 0; //!< filter mask for script events
    AngelScript::asIScriptModule* scriptModule = nullptr;
    AngelScript::asIScriptFunction* frameStepFunctionPtr = nullptr; //!< script function pointer to the frameStep function
    AngelScript::asIScriptFunction* eventCallbackFunctionPtr = nullptr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* eventCallbackExFunctionPtr = nullptr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* defaultEventCallbackFunctionPtr = nullptr; //!< script function pointer for spawner events
    ActorPtr associatedActor; //!< For ScriptCategory::ACTOR
    Ogre::String scriptName;
    Ogre::String scriptHash;
    Ogre::String scriptBuffer;
};

typedef std::map<ScriptUnitId_t, ScriptUnit> ScriptUnitMap;

struct LoadScriptRequest
{
    std::string lsr_filename; //!< Load from resource (file). If buffer is supplied, use this as display name only.
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
     * @param scriptname filename to load; if buffer is supplied, this is only a display name.
     * @param category How to treat the script?
     * @param associatedActor Only for category ACTOR
     * @param buffer String with full script body; if empty, a file will be loaded as usual.
     * @return Unique ID of the script unit (because one script file can be loaded multiple times).
     */
    ScriptUnitId_t loadScript(Ogre::String scriptname, ScriptCategory category = ScriptCategory::TERRAIN,
        ActorPtr associatedActor = nullptr, std::string buffer = "");

    /**
     * Unloads a script
     * @param unique_id The script unit ID as returned by `loadScript()`
     */
    void unloadScript(ScriptUnitId_t unique_id);

    /**
     * Calls the script's framestep function to be able to use timed things inside the script
     * @param dt time passed since the last call to this function in seconds
     * @return 0 on success, everything else on error
     */
    int framestep(Ogre::Real dt);

    void activateLogging();

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
    int executeString(Ogre::String command);

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
    */
    int addFunction(const Ogre::String& arg);

    /**
     * Checks if a global function exists
     * @param arg A declaration for the function.
    */
    int functionExists(const Ogre::String& arg);

    /**
     * Deletes a global function from the script
     * @param arg A declaration for the function.
    */
    int deleteFunction(const Ogre::String& arg);

    /**
     * Adds a global variable to the script
     * @param arg A declaration for the variable.
    */
    int addVariable(const Ogre::String& arg);

    /**
     * Deletes a global variable from the script
     * @param arg A declaration for the variable.
    */
    int deleteVariable(const Ogre::String& arg);

    int fireEvent(std::string instanceName, float intensity);

    void envokeCallback(int functionId, eventsource_t* source, NodeNum_t nodenum = NODENUM_INVALID, int type = 0);

    AngelScript::asIScriptEngine* getEngine() { return engine; };

    // method from Ogre::LogListener
    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    inline void SLOG(const char* msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro
    inline void SLOG(std::string msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro

    ScriptUnit& getScriptUnit(ScriptUnitId_t unique_id);
    ScriptUnitId_t getTerrainScriptUnit() const { return m_terrain_script_unit; } //!< @return SCRIPTUNITID_INVALID if none exists.
    ScriptUnitId_t getCurrentlyExecutingScriptUnit() const { return m_currently_executing_script_unit; } //!< @return SCRIPTUNITID_INVALID if none is executing right now.
    ScriptUnitMap const& getScriptUnits() const { return m_script_units; }

protected:

    /**
     * This function initialzies the engine and registeres all types
     */
    void init();

    /**
     * This is the callback function that gets called when script error occur.
     * When the script crashes, this function will provide you with more detail
     * @param msg arguments that contain details about the crash
     */
    void msgCallback(const AngelScript::asSMessageInfo* msg);

    Ogre::String composeModuleName(Ogre::String const& scriptName, ScriptCategory origin, ScriptUnitId_t id);

    /**
    * Helper for `loadScript()`, does the actual building without worry about unit management.
    * @return 0 on success, anything else on error.
    */
    int setupScriptUnit(int unit_id);

    AngelScript::asIScriptEngine* engine; //!< instance of the scripting engine
    AngelScript::asIScriptContext* context; //!< context in which all scripting happens
    Ogre::Log*      scriptLog;
    GameScript      m_game_script;
    ScriptUnitMap   m_script_units;
    ScriptUnitId_t  m_terrain_script_unit = SCRIPTUNITID_INVALID;
    ScriptUnitId_t  m_currently_executing_script_unit = SCRIPTUNITID_INVALID;
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
