
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

/// Argument #1 of script event `SE_ANGELSCRIPT_MANIPULATIONS`
enum angelScriptManipulationType
{
    ASMANIP_CONSOLE_SNIPPET_EXECUTED = 0, // 0 for Backwards compatibility.
    ASMANIP_SCRIPT_LOADED,                //!< Triggered after the script's `main()` completed; may trigger additional processing (for example, it delivers the *.mission file to mission system script). Args: #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 filename.
    ASMANIP_SCRIPT_UNLOADING,              //!< Triggered before unloading the script to let it clean up (important for missions). Args: #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 filename.
    ASMANIP_ACTORSIMATTR_SET              //!< Triggered when `setSimAttribute()` is called; additional args: #2 `RoR::ActorSimAtr`, #3 ---, #4 ---, #5 attr name, #6 value converted to string.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs