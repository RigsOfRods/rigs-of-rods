
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

namespace Script2Game {

/**
* Binding of RoR::ActorModifyRequest::Type; use with `MSG_SIM_MODIFY_ACTOR_REQUESTED`
*/
enum ActorModifyRequestType
{
    ACTOR_MODIFY_REQUEST_INVALID,
    ACTOR_MODIFY_REQUEST_RELOAD,               //!< Full reload from filesystem, requested by user
    ACTOR_MODIFY_REQUEST_RESET_ON_INIT_POS,
    ACTOR_MODIFY_REQUEST_RESET_ON_SPOT,
    ACTOR_MODIFY_REQUEST_SOFT_RESET,
    ACTOR_MODIFY_REQUEST_RESTORE_SAVED,        //!< Internal, DO NOT USE
    ACTOR_MODIFY_REQUEST_WAKE_UP
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs