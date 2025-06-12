
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
* Binding of `RoR::ScriptCategory` ~ for `game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED ...)`
*/
enum ScriptCategory
{
    SCRIPT_CATEGORY_INVALID,
    SCRIPT_CATEGORY_ACTOR,  //!< Defined in truck file under 'scripts', contains global variable `BeamClass@ thisActor`.
    SCRIPT_CATEGORY_TERRAIN, //!< Defined in terrn2 file under '[Scripts]', receives terrain eventbox notifications.
    SCRIPT_CATEGORY_CUSTOM, //!< Loaded by user via either: A) ingame console 'loadscript'; B) RoR.cfg 'diag_custom_scripts'; C) commandline '-runscript'.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs