namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Turboprop. Contains information specific to propeller engines.
 * @note Obtain the object using `BeamClass.getTurboprop(int index)`.
 */
class TurbopropClass
{
    // PLEASE maintain the same order as in 'bindings/TurbopropAngelscript.cpp' and 'physics/air/TurboProp.h'
public:
    /**
     * @return The propeller pitch, in degrees.
     */
    float getPropellerPitch();

    /**
     * @return The current torque applied to the propeller, in newton-metres.
     */
    float getPropellerIndicatedTorque();

    /**
     * @return The current torque that can be applied to the propeller, in newton-metres.
     */
    float getPropellerMaxTorque();

    /**
     * @return The maximum power of the engine, in kilowatts.
     */
    float getPropellerMaxPower();

    /**
     * @return Whether the engine is a piston propeller.
     */
    bool isPistonProp();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
