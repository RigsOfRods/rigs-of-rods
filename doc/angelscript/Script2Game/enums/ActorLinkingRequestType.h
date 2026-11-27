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
 * Binding of RoR::ActorLinkingRequestType, for use with MSG_SIM_ACTOR_LINKING_REQUESTED.
 */
enum ActorLinkingRequestType 
{
    ACTORLINKINGREQUESTTYPE_INVALID,
    ACTORLINKINGREQUESTTYPE_HOOK_LOCK,          //!< Lock hooks of 'hook_group' (-1 = all)
    ACTORLINKINGREQUESTTYPE_HOOK_UNLOCK,        //!< Unlock hooks of 'hook_group' (-1 = all)
    ACTORLINKINGREQUESTTYPE_HOOK_TOGGLE,        //!< Toggle hooks of 'hook_group' (-1 = all), like EV_COMMON_LOCK
    ACTORLINKINGREQUESTTYPE_HOOK_MOUSE_TOGGLE,  //!< Toggle the single hook at node 'hook_mousenode', like clicking it with the mouse
    ACTORLINKINGREQUESTTYPE_TIE_TOGGLE,         //!< Toggle ties of 'tie_group' (-1 = all), like EV_COMMON_SECURE_LOAD
    ACTORLINKINGREQUESTTYPE_ROPE_TOGGLE,        //!< Toggle ropes of 'rope_group' (-1 = all)
    ACTORLINKINGREQUESTTYPE_SLIDENODE_TOGGLE,   //!< Toggle slide node lock
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
