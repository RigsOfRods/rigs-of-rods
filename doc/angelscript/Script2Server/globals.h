
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //


/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Server
 *  @{
 */   

namespace Script2Server
{
    
    /**
    * The server configuration, use [server.get_serverMode()](@ref Script2Server::ServerScriptClass::get_serverMode)
    */
    enum ServerType
    {
        SERVER_LAN,
        SERVER_INET,
        SERVER_AUTO
    };

    /**
    * This is returned by the `playerChat()/streamAdded()` callback and determines how the message is treated;<br>order: least restrictive to most restrictive!
    */
    enum broadcastType 
    {
        BROADCAST_AUTO = -1,  //!< Do not edit the publishmode (for scripts only)
        BROADCAST_ALL,        //!< broadcast to all clients including sender
        BROADCAST_NORMAL,     //!< broadcast to all clients except sender
        BROADCAST_AUTHED,     //!< broadcast to authed users (bots)
        BROADCAST_BLOCK       //!< no broadcast
    };

    TO_ALL = -1; //!< constant for functions that receive an uid for sending something
    
    /**
    *  Use with [server.say()](@ref Script2Server::ServerScriptClass::say) to specify who said it.
    */
    enum serverSayType 
    {
        FROM_SERVER = 0,
        FROM_HOST,
        FROM_MOTD,
        FROM_RULES
    };
    
    /**
    * Used with [`curlStatus()` callback](@ref Server2Script::curlStatus)
    */    
    enum CurlStatusType
    {
        CURL_STATUS_INVALID,  //!< Should never be reported.
        CURL_STATUS_START,    //!< New CURL request started, n1/n2 both 0.
        CURL_STATUS_PROGRESS, //!< Download in progress, n1 = bytes downloaded, n2 = total bytes.
        CURL_STATUS_SUCCESS,  //!< CURL request finished, n1 = CURL return code, n2 = HTTP result code, message = received payload.
        CURL_STATUS_FAILURE,  //!< CURL request finished, n1 = CURL return code, n2 = HTTP result code, message = CURL error string.
    };

} // namespace Script2Server

/// @}    //addtogroup Script2Server
/// @}    //addtogroup ScriptSideAPIs
