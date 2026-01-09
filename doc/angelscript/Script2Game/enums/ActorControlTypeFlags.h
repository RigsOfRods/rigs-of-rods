
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
 * Types of controls for an actor. It is a binding of RoR::ActorControlTypeFlags.
 * 
 * The values are bit flags used to indicate which controls
 * are affected by external input sources, such as a keyboard.
 */
enum ActorControlTypeFlags
{
    ACT_NO_CONTROLS = 0,        //!< No controls are affected by external inputs.
    ACT_THROTTLE = 1,           //!< Car/aircraft/boat throttle is affected by external inputs.
    ACT_CLUTCH = 2,             //!< The clutch is affected by external inputs.
    ACT_BRAKE = 4,              //!< Car/aircraft brakes are affected by external inputs.
    ACT_STEERING = 8,           //!< The steering is affected by external inputs.
    ACT_AILERON = 16,           //!< The aileron is affected by external inputs.
    ACT_ELEVATOR = 32,          //!< The elevator is affected by external inputs.
    ACT_RUDDER = 64,            //!< The rudder is affected by external inputs.
    ACT_ALL_CONTROLS = 128 - 1  //!< All the controls are affected by external inputs.
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs