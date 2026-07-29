
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
    SCRIPT_CATEGORY_ACTOR,   //!< Defined in truck file under 'scripts', contains global variable `BeamClass@ thisActor`.
    SCRIPT_CATEGORY_TERRAIN, //!< Defined in terrn2 file under '[Scripts]', receives terrain eventbox notifications.
    SCRIPT_CATEGORY_GADGET,  //!< Associated with a .gadget mod file, launched via UI or any method given below for CUSTOM scripts (use .gadget suffix - game will fix up category to `GADGET`).
    SCRIPT_CATEGORY_AI_BOT,  //!< Only valid under `RoR::MpState::LOCAL_SCRIPT`; registers as RoRnet user in the server script (i.e. for competitive racing).
    SCRIPT_CATEGORY_CUSTOM,  //!< Loaded by user via either: A) ingame console 'loadscript'; B) RoR.cfg 'diag_custom_scripts'; C) commandline '-runscript'.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs