
// =============================================================================
// This file is adopted from rorserver at commit 4a7109ae2d9a081ccfdad8cc696dc54efe49acb3
// Changes from the original are marked with "//RIGSOFRODS"
// =============================================================================

/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2007   Pierre-Michel Ricordel
Copyright 2014+  Rigs of Rods Community

"Rigs of Rods Server" is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

"Rigs of Rods Server" is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef USE_ANGELSCRIPT

#include "ServerScriptCurlHelpers.h"
#include "ServerScriptLogger.h"
#include "ServerScriptSequencer.h"
#include "ServerScript.h"
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "angelscript.h"
#include "RoRnet.h"

namespace RoR { // RIGSOFRODS

/**
 * This struct holds the information for a script callback.
 */
struct rorserver_callback_t {
    AngelScript::asIScriptObject *obj{nullptr};  //!< The object instance that will need to be used with the function.
    AngelScript::asIScriptFunction *func{nullptr}; //!< The function or method pointer that will be called.
};
typedef std::vector<rorserver_callback_t> callbackList;


class ServerScriptEngine {
public:
    enum class ThreadState
    {
        NOT_RUNNING,
        RUNNING,
        STOP_REQUESTED
    };

    ServerScriptEngine(ServerScriptSequencer* sequencer);

    ~ServerScriptEngine();

    int loadScript(std::string scriptName);

    void unloadScript(); // RIGSOFRODS: Unload the script (only one can run at a time).

    /// @name callbacks
    /// @{

    void playerDeleted(int uid, int crash = 0);

    void playerAdded(RoRnet::UserInfo& user);

    int streamAdded(int uid, RoRnet::StreamRegister *reg);

    int playerChat(int uid, std::string msg);

    void gameCmd(int uid, const std::string &cmd);

    /**
     * Params `n1`, `n2` and `message` depend on status type :
     * - CURL_STATUS_PROGRESS: n1 = bytes downloaded, n2 = total bytes, message = empty
     * - CURL_STATUS_SUCCESS: n1 = CURL return code, n2 = HTTP result code, message = payload as string
     * - CURL_STATUS_FAILURE: n1 = CURL return code, n2 = HTTP result code, message = CURL error string
     */
    void curlStatus(ServerScriptCurlStatusType type, int n1, int n2, std::string displayname, std::string message);

    int frameStep(float dt);

    /// @}

    /**
     * Gets the currently used AngelScript script engine.
     * @return a pointer to the currently used AngelScript script engine
     */
    AngelScript::asIScriptEngine *getEngine() { return engine; };

    /**
     * Sets an exception that aborts the currently running script and shows the exception in the log file.
     * @param message A descriptive error message.
     */
    void setException(const std::string &message);

    /**
     * Adds a script callback.
     * @param type The type of the callback. This can be one of the following: 'frameStep', 'playerChat', 'gameCmd', 'playerAdded', 'playerDeleted'.
     * @param func A pointer to a script function.
     * @param obj A pointer to the object of the method or NULL if func is a global function.
     */
    void addCallback(const std::string &type, AngelScript::asIScriptFunction *func, AngelScript::asIScriptObject *obj);

    /**
     * This method checks and converts the parameters and then adds a script callback.
     * @param type The type of the callback. \see addCallback
     * @param func The name of a script function.
     * @param obj A pointer to the object of the method or NULL if func is a global function.
     */
    void addCallbackScript(const std::string &type, const std::string &func, AngelScript::asIScriptObject *obj);

    /**
     * Deletes a script callback.
     * @param type The type of the callback. \see addCallback
     * @param func A pointer to a script function.
     * @param obj A pointer to the object of the method or NULL if func is a global function.
     */
    void deleteCallback(const std::string &type, AngelScript::asIScriptFunction *func, AngelScript::asIScriptObject *obj);

    /**
     * This method checks and converts the parameters and then deletes a script callback.
     * @param type The type of the callback. \see addCallback
     * @param func The name of a script function.
     * @param obj A pointer to the object of the method or NULL if func is a global function.
     */
    void deleteCallbackScript(const std::string &type, const std::string &_func, AngelScript::asIScriptObject *obj);

    /**
     * Deletes all script callbacks.
     */
    void deleteAllCallbacks();

    /**
     * This checks if a script callback exists.
     * @param type The type of the callback. \see addCallback
     * @param func A pointer to a script function.
     * @param obj A pointer to the object of the method or NULL if func is a global function.
     * @return true if the callback exists
     */
    bool callbackExists(const std::string &type, AngelScript::asIScriptFunction *func, AngelScript::asIScriptObject *obj);

    // Timer thread control
    void        EnsureTimerThreadRunning();
    void        StopTimerThread();
    ThreadState GetTimerThreadState();

protected:
    ServerScriptSequencer* seq;
    AngelScript::asIScriptEngine *engine;                //!< instance of the scripting engine
    AngelScript::asIScriptContext *context;              //!< context in which all scripting happens
    std::map<std::string, callbackList> callbacks; //!< A map containing the script callbacks by type.

    // Timer thread context
    std::thread m_timer_thread;
    ThreadState m_timer_thread_state = ThreadState::NOT_RUNNING;
    std::mutex  m_timer_thread_mutex;

    /**
     * This function initialzies the engine and registeres all types
     */
    void init();

    /**
     * This is the callback function that gets called when script error occur.
     * When the script crashes, this function will provide you with more detail
     * @param msg arguments that contain details about the crash
     * @param param unkown?
     */
    void msgCallback(const AngelScript::asSMessageInfo *msg);

    /**
     * This callback gets called when an exception occurs in the script.
     * It logs the exception message together with the place in the script where the error occurs.
     * @param ctx The context in which the exception ocurred.
     * @param param An unused parameter.
     */
    void ExceptionCallback(AngelScript::asIScriptContext *ctx, void *param);

    /**
     * This logs all variables and their values at the specified stack level.
     * @param ctx The context that should be used.
     * @param stackLevel A number representing the level in the stack that should be logged.
     */
    void PrintVariables(AngelScript::asIScriptContext *ctx, int stackLevel);

    /**
     * unused
     */
    void LineCallback(AngelScript::asIScriptContext *ctx, void *param);

    /**
     * A loop that periodically the frameStep() script callback.
     */
    void TimerThreadMain();
};

} // namespace RoR

#endif // USE_ANGELSCRIPT

