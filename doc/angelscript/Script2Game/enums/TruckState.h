
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
 * Binding of RoR::ActorState
 */
enum TruckState
{
	TS_SIMULATED,      //!< locally simulated and active
	TS_SLEEPING,       //!< locally simulated but sleeping
	TS_NETWORKED,      //!< controlled by network data
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs