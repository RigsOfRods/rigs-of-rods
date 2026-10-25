
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
 * Binding of RoR::ActorType
 */
enum truckTypes
{
    TT_NOT_DRIVEABLE,
    TT_TRUCK,
    TT_AIRPLANE,
    TT_BOAT,
    TT_MACHINE,
    // TT_AI ~ replaced by checking if `BeamClass::getVehicleAI()` returns a valid object.
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs