

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Server
 *  @{
 */   

namespace Script2Server {

/**
*  This is used to define who says it, when the server says something
*/
enum serverSayType 
{
    FROM_SERVER = 0,
    FROM_HOST,
    FROM_MOTD,
    FROM_RULES
}

/**
* This is returned by the `playerChat()/streamAdded()` callback and determines how the message is treated.
*/
enum broadcastType 
{
    // order: least restrictive to most restrictive!
    BROADCAST_AUTO = -1,  //!< Do not edit the publishmode (for scripts only)
    BROADCAST_ALL,        //!< broadcast to all clients including sender
    BROADCAST_NORMAL,     //!< broadcast to all clients except sender
    BROADCAST_AUTHED,     //!< broadcast to authed users (bots)
    BROADCAST_BLOCK       //!< no broadcast
};

TO_ALL = -1 //!< constant for functions that receive an uid for sending something

} // namespace Script2Server

/// @}    //addtogroup Script2Server
/// @}    //addtogroup ScriptSideAPIs
