/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include <Ogre.h>

#include "RoRPrerequisites.h"

#include "InterThreadStoreVector.h"
#include "Singleton.h"
#include "RigDef_File.h"

#include "scriptdictionary/scriptdictionary.h"
#include "scriptbuilder/scriptbuilder.h"

#include <angelscript.h>
/**
 * @file ScriptEngine.h
 * @version 0.1.0
 * @brief AngelScript interface to the game
 * @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)
 */

class GameScript;

/**
 *  @brief Utility wrapper for game scripts
 */
struct ScriptUnit
{
    AngelScript::asIScriptModule*   su_module   = nullptr; //!< Bytecode
    AngelScript::asIScriptFunction* su_loop_fn  = nullptr;
    AngelScript::asIScriptContext*  su_context  = nullptr; //!< Stack
};

/**
 *  @brief This class represents the angelscript scripting interface. It can load and execute scripts.
 */
class ScriptEngine : public RoRSingletonNoCreation<ScriptEngine>, public Ogre::LogListener, public ZeroedMemoryAllocator
{
    friend class GameScript;

public:

    ScriptEngine(Collisions* _coll = nullptr);
    ~ScriptEngine();

    /**
     * Loads a script associated with terrn2
     * @param scriptname filename to load
     * @return 0 on success, everything else on error
     */
    int loadTerrainScript(Ogre::String scriptname);

    /**
     * Loads a script associated with actor (truck/load...)
     */
    bool loadActorScript(Actor* actor, RigDef::Script& def);

    /**
     * Calls the script's framestep function to be able to use timed things inside the script
     * @param dt time passed since the last call to this function in seconds
     * @return 0 on success, everything else on error
     */
    int framestep(Ogre::Real dt);

    void activateLogging();

    unsigned int eventMask; //!< filter mask for script events

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

    Ogre::StringVector getAutoComplete(Ogre::String command);

    int fireEvent(std::string instanceName, float intensity);

    int envokeCallback(int functionId, eventsource_t* source, node_t* node = 0, int type = 0);

    AngelScript::asIScriptEngine* getEngine() { return engine; };

    Ogre::String getScriptName() { return scriptName; };
    Ogre::String getScriptHash() { return scriptHash; };

    // method from Ogre::LogListener
    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    inline void SLOG(const char* msg) { this->scriptLog->logMessage(msg); } ///< Replacement of macro
    inline void SLOG(std::string msg) { this->scriptLog->logMessage(msg); } ///< Replacement of macro


protected:

    Collisions* coll;
    AngelScript::asIScriptEngine* engine; //!< instance of the scripting engine - legacy framestep logic
    AngelScript::asIScriptEngine* m_engine_frame; //!< instance of the scripting engine - framestep logic (asynchronous with simulation)
    AngelScript::asIScriptContext* context; //!< context in which all scripting happens
    AngelScript::asIScriptFunction* frameStepFunctionPtr; //!< script function pointer to the frameStep function
    AngelScript::asIScriptFunction* eventCallbackFunctionPtr; //!< script function pointer to the event callback function
    AngelScript::asIScriptFunction* defaultEventCallbackFunctionPtr; //!< script function pointer for spawner events
    Ogre::String scriptName;
    Ogre::String scriptHash;
    Ogre::Log* scriptLog;

    InterThreadStoreVector<Ogre::String> stringExecutionQueue; //!< The string execution queue \see queueStringForExecution

    static const char* moduleName;

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
};

// This function will register the following objects with the scriptengine:
//    - Ogre::Vector3
//    - Ogre::Vector2
//    - Ogre::Radian
//    - Ogre::Degree
//    - Ogre::Quaternion
//    - Ogre::ColourValue
// Defined in file 'OgreAngelscript.cpp'
void RegisterOgreObjects(AngelScript::asIScriptEngine* engine);

// Defined in file 'ImGuiAngelscript.cpp'
void RegisterImGuiBindings(AngelScript::asIScriptEngine* engine);

// Defined in file 'FrameStepAngelscript.cpp'
void RegisterFrameStepInterface(AngelScript::asIScriptEngine* engine);

#endif // USE_ANGELSCRIPT
