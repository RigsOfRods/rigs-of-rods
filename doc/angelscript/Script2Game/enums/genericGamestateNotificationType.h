
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

/// Argument $1 of script event `RoR::SE_GENERIC_GAMESTATE_NOTIFICATION`
enum genericGamestateNotificationType
{
    GAMESTATE_NOTIFICATION_NONE = 0,
    GAMESTATE_RACETRACK_LOAD_REQUESTED,       //!< Instructs race manager (TERRAIN script) to load a '.racetrack' mod; args: $5 mission filename, $6 mission rg name
    GAMESTATE_RACETRACK_UNLOAD_REQUESTED,     //!< Instructs race manager to unload race; args: $2 raceID
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs