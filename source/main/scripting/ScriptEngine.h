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

#define TRIGGER_EVENT(x, y) App::GetScriptEngine()->triggerEvent((x), (y))

#include "Application.h"
#include "GameScript.h"
#include "InterThreadStoreVector.h"
#include "ScriptEvents.h"

#include <Ogre.h>
#include "scriptdictionary/scriptdictionary.h"
#include "scriptbuilder/scriptbuilder.h"

namespace RoR {

enum class ScriptOrigin
{
    INVALID,
    TERRAIN,
    ADDON
};

struct ScriptUnit
{
    ScriptOrigin scriptOrigin = ScriptOrigin::INVALID;
    unsigned int eventMask = 0; //!< filter mask for script events
    AngelScript::asIScriptModule* scriptModule = nullptr;
    AngelScript::asIScriptFunction* frameStepFunctionPtr = nullptr; //!< script function pointer to the frameStep function
    AngelScript::asIScriptFunction* eventCallbackFunctionPtr = nullptr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* defaultEventCallbackFunctionPtr = nullptr; //!< script function pointer for spawner events
    Ogre::String scriptName;
    Ogre::String scriptHash;
};

typedef std::vector<ScriptUnit> ScriptUnitVec;

/**
 *  @brief This class represents the angelscript scripting interface. It can load and execute scripts.
 *  @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)
 */
class ScriptEngine : public Ogre::LogListener, public ZeroedMemoryAllocator
{
    friend class GameScript;

public:

    ScriptEngine();
    ~ScriptEngine();

    /**
     * Loads a script
     * @param scriptname filename to load
     * @return 0 on success, everything else on error
     */
    int loadScript(Ogre::String scriptname, ScriptOrigin origin = ScriptOrigin::TERRAIN);

    /**
     * Unloads a script
     * @param scriptname filename to unload
     */
    void unloadScript(Ogre::String scriptname, ScriptOrigin origin);

    /**
     * Calls the script's framestep function to be able to use timed things inside the script
     * @param dt time passed since the last call to this function in seconds
     * @return 0 on success, everything else on error
     */
    int framestep(Ogre::Real dt);

    void activateLogging();

    /**
     * triggers an event. Not to be used by the end-user
     * @param eventValue \see enum scriptEvents
     */
    void triggerEvent(int scriptEvents, int value = 0);

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

    int envokeCallback(int functionId, eventsource_t* source, node_t* node = 0, int type = 0);

    AngelScript::asIScriptEngine* getEngine() { return engine; };

    // method from Ogre::LogListener
    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    inline void SLOG(const char* msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro
    inline void SLOG(std::string msg) { this->scriptLog->logMessage(msg); } //!< Replacement of macro

    ScriptUnitVec& getScriptUnits() { return m_script_units; }
    int getTerrainScriptUnit() const { return m_terrain_script_unit; } //!< @return -1 if none exists.
    int getCurrentlyExecutingScriptUnit() const { return m_currently_executing_script_unit; } //!< @return -1 if none is executing right now.


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

    Ogre::String composeModuleName(Ogre::String const& scriptName, ScriptOrigin origin);

    AngelScript::asIScriptEngine* engine; //!< instance of the scripting engine
    AngelScript::asIScriptContext* context; //!< context in which all scripting happens
    Ogre::Log*      scriptLog;
    GameScript      m_game_script;
    ScriptUnitVec   m_script_units;
    int             m_terrain_script_unit = -1;
    int             m_currently_executing_script_unit = -1;

    InterThreadStoreVector<Ogre::String> stringExecutionQueue; //!< The string execution queue \see queueStringForExecution
};

// This function will register the following objects with the scriptengine:
//    - Ogre::Vector3
//    - Ogre::Vector2
//    - Ogre::Radian
//    - Ogre::Degree
//    - Ogre::Quaternion
//    - Ogre::ColourValue
void RegisterOgreObjects(AngelScript::asIScriptEngine* engine);

void RegisterImGuiBindings(AngelScript::asIScriptEngine* engine);

} // namespace RoR

#else // USE_ANGELSCRIPT
#   define TRIGGER_EVENT(x, y)
#endif // USE_ANGELSCRIPT
