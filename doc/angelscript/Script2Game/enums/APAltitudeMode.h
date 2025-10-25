
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
 * Altitude modes for the autopilot system
 */
enum APAltitudeMode
{
	ALT_NONE,		//!< Disables all the altitude modes.
	ALT_FIXED,		//!< The autopilot keeps the aircraft at a constant altitude set by `AutopilotClass.adjustAltitude(int addedAltitude)`.
	ALT_VS			//!< The autopilot keeps the aircraft at a constant rate of climb/descent set by `AutopilotClass.adjustVerticalSpeed(int addedVS)`.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs