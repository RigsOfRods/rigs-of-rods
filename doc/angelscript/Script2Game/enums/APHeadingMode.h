
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
 * Heading modes for the autopilot system
 */
enum APHeadingModes
{
	HEADING_NONE,		//!< Disables all the heading modes.
	HEADING_FIXED,		//!< The autopilot flies a heading set by `AutopilotClass.adjustHeading(int addedHeading)`
	HEADING_NAV,		//!< The autopilot flies an instrument landing system (ILS) approach if available. If no ILS is available, the autopilot is disconnected.
	HEADING_WLV			//!< The autopilot keeps the wings level.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs