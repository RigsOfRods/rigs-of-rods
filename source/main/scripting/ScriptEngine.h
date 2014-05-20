/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created on 24th of February 2009 by Thomas Fischer

#ifndef SCRIPTENGINE_H__
#define SCRIPTENGINE_H__

#include "RoRPrerequisites.h"

#include "Collisions.h"
#include "InterThreadStoreVector.h"
#include "Ogre.h"
#include "Singleton.h"

#include "scriptdictionary/scriptdictionary.h"
#include "scriptbuilder/scriptbuilder.h"

#define AS_INTERFACE_VERSION "0.2.0" //!< versioning for the scripting interface

#define SLOG(x) ScriptEngine::getSingleton().scriptLog->logMessage(x);

/**
 * @file ScriptEngine.h
 * @version 0.1.0
 * @brief AngelScript interface to the game
 * @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)
 */

class GameScript;
/**
 *  @brief This class represents the angelscript scripting interface. It can load and execute scripts.
 */
class ScriptEngine : public RoRSingletonNoCreation<ScriptEngine>, public Ogre::LogListener, public ZeroedMemoryAllocator
{
	friend class GameScript;

public:

	ScriptEngine(RoRFrameListener *efl, Collisions *_coll);
	~ScriptEngine();

	void setCollisions(Collisions *_coll) { coll=_coll; };

	/**
	 * Loads a script
	 * @param scriptname filename to load
	 * @return 0 on success, everything else on error
	 */
	int loadScript(Ogre::String scriptname);

	/**
	 * Calls the script's framestep function to be able to use timed things inside the script
	 * @param dt time passed since the last call to this function in seconds
	 * @return 0 on success, everything else on error
	 */
	int framestep(Ogre::Real dt);

	void activateLogging();

	unsigned int eventMask;                              //!< filter mask for script events
	
	/**
	 * triggers an event. Not to be used by the end-user
	 * @param eventValue \see enum scriptEvents
	 */
	void triggerEvent(int scriptEvents, int value=0);

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

	int envokeCallback(int functionPtr, eventsource_t *source, node_t *node=0, int type=0);

	AngelScript::asIScriptEngine *getEngine() { return engine; };

	Ogre::String getScriptName() { return scriptName; };
	Ogre::String getScriptHash() { return scriptHash; };

	// method from Ogre::LogListener
#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
		void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName);
#else
		void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage);
#endif // OGRE_VERSION

	// deprecated
	void exploreScripts();


	Ogre::Log *scriptLog;

protected:

    RoRFrameListener *mefl;                 //!< local RoRFrameListener instance, used as proxy for many functions
	Collisions *coll;
    AngelScript::asIScriptEngine *engine;   //!< instance of the scripting engine
	AngelScript::asIScriptContext *context; //!< context in which all scripting happens
	int frameStepFunctionPtr;               //!< script function pointer to the frameStep function
	int wheelEventFunctionPtr;              //!< script function pointer
	int eventCallbackFunctionPtr;           //!< script function pointer to the event callback function
	int defaultEventCallbackFunctionPtr;    //!< script function pointer for spawner events
	Ogre::String scriptName;
	Ogre::String scriptHash;
	std::map <std::string , std::vector<int> > callbacks;
	
	InterThreadStoreVector< Ogre::String > stringExecutionQueue;     //!< The string execution queue \see queueStringForExecution

	static const char *moduleName;


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
	 * This function reads a file into the provided string.
	 * @param filename filename of the file that should be loaded into the script string
	 * @param script reference to a string where the contents of the file is written to
	 * @param hash reference to a string where the hash of the contents is written to
	 * @return 0 on success, everything else on error
	 */
	int loadScriptFile(const char *fileName, std::string &script, std::string &hash);

	// undocumented debugging functions below, not working.
	void ExceptionCallback(AngelScript::asIScriptContext *ctx, void *param);
	void PrintVariables(AngelScript::asIScriptContext *ctx, int stackLevel);
	void LineCallback(AngelScript::asIScriptContext *ctx, unsigned long *timeOut);
};


#endif //SCRIPTENGINE_H__
