
namespace Server2Script {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Server2Script
 *  @{
 */   

/*
Callbacks
---------

The game recognizes the following callbacks. 
You can register them either by using a global function of the same name
or by manually invoking `server.setCallback()`. Note that some callbacks allow
multiple handler functions (`setCallback()` adds another) while other can only
have a single handler function (`setCallback()` replaces the previous). 
*/

/** Required; Script setup function - invoked once when script is loaded.
*   If not present, the server will report error and abandon the script.
*/
void main();

/**
*  executed periodically, the parameter is delta time (time since last execution) in milliseconds.
*/
void frameStep(float dt_millis);

/**
* executed when player joins.
*/
void playerAdded(int uid);

/**
* executed when player leaves.
*/ 
void playerDeleted(int uid, int crashed);

/**
* executed when player spawns an actor; Returns `broadcastType` which determines how the message is treated.
*/ 
int streamAdded(int uid, StreamRegister@ reg);

/**
* ONLY ONE AT A TIME ~ executed when player sends a chat message; Returns `broadcastType` which determines how the message is treated.
*/ 
int playerChat(int uid, const string &in msg);

/**
* ONLY ONE AT A TIME ~ invoked when a script running on client calls `game.sendGameCmd()`
*/  
void gameCmd(int uid, const string &in cmd);

/** @}*/   //addtogroup Server2Script
/** @}*/   //addtogroup ScriptSideAPIs

} // namespace Server2Script