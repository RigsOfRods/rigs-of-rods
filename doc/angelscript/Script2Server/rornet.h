
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //


/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Server
 *  @{
 */   

namespace Script2Server {
    
/**
*  Binding of RoRnet::UserAuth
*/
enum authType
{
    AUTH_NONE   = 0,                   //!< no authentication
    AUTH_ADMIN  = BITMASK(1),          //!< admin on the server
    AUTH_RANKED = BITMASK(2),          //!< ranked status
    AUTH_MOD    = BITMASK(3),          //!< moderator status
    AUTH_BOT    = BITMASK(4),          //!< bot status
    AUTH_BANNED = BITMASK(5)           //!< banned
};


/** Binding of RoRnet::StreamRegister class
*/
class StreamRegister
{
    string getName();
    int type;
    int status;
    int origin_sourceid;
    int origin_streamid;
};

} // namespace Script2Server

/// @}    //addtogroup Script2Server
/// @}    //addtogroup ScriptSideAPIs
