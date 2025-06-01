
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
 * Binding of RoR::FreeForceType, for use with MSG_SIM_ADD_FREEFORCE_REQUESTED.
 */
enum FreeForceType 
{
    FREEFORCETYPE_DUMMY,
    FREEFORCETYPE_CONSTANT,
    FREEFORCETYPE_TOWARDS_COORDS,
    FREEFORCETYPE_TOWARDS_NODE,
    FREEFORCETYPE_HALFBEAM_GENERIC,
    FREEFORCETYPE_HALFBEAM_ROPE,
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs