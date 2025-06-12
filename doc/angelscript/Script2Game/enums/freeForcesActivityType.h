
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

/// Argument #1 of script event `RoR::SE_GENERIC_FREEFORCES_ACTIVITY`
enum freeForcesActivityType
{
    FREEFORCESACTIVITY_NONE,

    FREEFORCESACTIVITY_ADDED,
    FREEFORCESACTIVITY_MODIFIED,
    FREEFORCESACTIVITY_REMOVED,

    FREEFORCESACTIVITY_DEFORMED, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the actual stress, arg #6 (string containing float) maximum stress.
    FREEFORCESACTIVITY_BROKEN, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the applied force, arg #6 (string containing float) breaking threshold force.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs