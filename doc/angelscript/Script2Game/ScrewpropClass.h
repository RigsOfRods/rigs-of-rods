namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Screwprop. Contains information and control functions for a boat engine (screwprop).
 * @note Obtain the object using `BeamClass.getScrewprop(int index)`.
 */
class ScrewpropClass
{
    // PLEASE maintain the same order as in 'bindings/ScrewpropAngelscript.cpp' and 'physics/water/ScrewProp.h'
public:
    /**
     * @brief Sets the throttle.
     * @param throttle Ranges from 0 (idle) to 1 (full throttle).
     */
    void setThrottle(float throttle);

    /**
     * @brief Sets the rudder deflection.
     * @param rudderDeflection The rudder deflection, expressed as a factor of the deflection angle.
     * @note As an example, -0.25 means 90° rudder to the right, 0.25 means 90° rudder to the left.
     */
    void setRudder(float rudderDeflection);

    /**
     * @return The current throttle value, from 0 to 1.
     */
    float getThrottle();

    /**
     * @return The current rudder deflection. See `setRudder(float rudderDeflection)` for details.
     */
    float getRudder();

    /**
     * @return The maximum force the screwprop can apply, in newtons.
     */
    float getMaxPower();

    /**
     * @return Whether the screwprop is in reverse.
     */
    bool getReverse();

    /**
     * @return Toggles the reverse.
     */
    void toggleReverse();

    /**
     * @returns The reference node number in the N/B.
     */
    int getRefNode();

    /**
     * @returns The node number in the N/B for the back of the screwprop.
     */
    int getBackNode();

    /**
     * @returns The node number in the N/B for the top of the screwprop.
     */
    int getUpNode();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
