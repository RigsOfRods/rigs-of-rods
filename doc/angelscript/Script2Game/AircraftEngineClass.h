namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::AeroEngine. Contains information and control functions for an aircraft engine (turbojet, propeller).
 * @note Obtain the object using `BeamClass.getAircraftEngine(int index)`.
 */
class AircraftEngineClass
{
    // PLEASE maintain the same order as in 'bindings/AircraftEngineClassAngelscript.cpp' and 'physics/air/AeroEngine.h'
public:
    /**
     * @brief Sets the throttle.
     * @param throttle Ranges from 0 (idle thrust) to 1 (full thrust).
     */
    void setThrottle(float throttle);

    /**
     * @return The current throttle value, from 0 to 1.
     */
    float getThrottle();

    /**
     * @brief Toggles the thrust reversers.
     */
    void toggleReverse();

    /**
     * @brief Sets the thrust reverser status.
     */
    void setReverse(bool reverse);

    /**
     * @return the thrust reverser status.
     */
    bool getReverse();

    /**
     * @brief Toggles the engine ignition.
     */
    void flipStart();

    /**
     * @brief Gets the current RPM value, as a percentage.
     */
    float getRPMPercent();

    /**
     * @returns Whether the engine has failed.
     */
    bool isFailed();

    /**
     * @brief Gets the engine type.
     */
    AircraftEngineTypes getType();

    /**
     * @returns The ignition status.
     */
    bool getIgnition();

    /**
     * @returns The node number for the front of the engine, or 65535 (invalid node) if no node is set.
     */
    int getFrontNode();

    /**
     * @returns The node number for the back of the engine, or 65535 (invalid node) if no node is set.
     */
    int getBackNode();

    /**
     * @returns Whether the engine is warming up.
     */
    bool getWarmup();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
