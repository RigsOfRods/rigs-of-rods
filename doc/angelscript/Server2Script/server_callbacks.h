
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

namespace Server2Script {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Server2Script
 *  @{
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
* executed when player spawns an actor; Returns [`broadcastType`](@ref Script2Server::broadcastType) which determines how the message is treated.
*/ 
int streamAdded(int uid, StreamRegister@ reg);

/**
* ONLY ONE AT A TIME ~ executed when player sends a chat message; Returns [`broadcastType`](@ref Script2Server::broadcastType) which determines how the message is treated.
*/ 
int playerChat(int uid, const string &in msg);

/**
* ONLY ONE AT A TIME ~ invoked when a script running on client calls [`game.sendGameCmd()`](@ref Script2Game::GameScriptClass::sendGameCmd).
*/  
void gameCmd(int uid, const string &in cmd);

/**
* Reports status of background CURL request - use [`server.curlRequestAsync()`](@ref Script2Server::ServerScriptClass::curlRequestAsync) to invoke one.
* @param type [`CurlStatusType`](@ref Script2Server::CurlStatusType) Determines meaning of `n1`/`n2` params.
*/
void curlStatus(Script2Server::CurlStatusType type, int n1, int n2, string displayname, string message);

/** @}*/   //addtogroup Server2Script
/** @}*/   //addtogroup ScriptSideAPIs

} // namespace Server2Script