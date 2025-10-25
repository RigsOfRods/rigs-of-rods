namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Autopilot. Controls the autopilot system.
 * @note Obtain the object using `BeamClass.getAutopilot()`.
 */
class AutopilotClass
{
    // PLEASE maintain the same order as in 'bindings/AutopilotAngelscript.cpp' and 'gameplay/AutoPilot.h'
public:
    /**
     * @brief Disconnects the autopilot.
     */
    void disconnect();

    /**
     * @brief Sets or toggles the heading modes.
     * @param headingMode The heading mode. If the autopilot is in the same mode as `headingMode`, it toggles it. Otherwise, it sets the new mode.
     */
    APHeadingMode toggleHeading(APHeadingMode headingMode);

    /**
     * @brief Sets or toggles the altitude modes.
     * @param altitudeMode The altitude mode. If the autopilot is in the same mode as `altitudeMode`, it toggles it. Otherwise, it sets the new mode.
     */
    APAltitudeMode toggleAltitude(APAltitudeMode altitudeMode);

    /**
     * @brief Toggles the auto-throttle.
     * 
     * The auto-throttle keeps a constant speed set by `adjustIAS(int addedIAS)`
     */
    bool toggleIAS();

    /**
     * @brief Toggles the ground proximity warning system (GPWS).
     */
    bool toggleGPWS();

    /**
     * @brief Adds `addedHeading` to the current heading.
     * @param addedHeading The heading (in degrees) that will be added to the current heading. Use negative numbers to subtract.
     */
    int adjustHeading(int addedHeading);

    /**
     * @brief Adds `addedAltitude` to the current altitude.
     * @param addedAltitude The altitude (in feet) that will be added to the current altitude. Use negative numbers to subtract.
     */
    int adjustAltitude(int addedAltitude);

    /**
     * @brief Adds `addedVS` to the current vertical speed.
     * @param addedVS The vertical speed (in feet per minute) that will be added to the current vertical speed. Use negative numbers to subtract.
     */
    int adjustVerticalSpeed(int addedVS);

    /**
     * @brief Adds `addedIAS` to the current airspeed reference for the auto-throttle.
     * @param addedVS The airspeed (in knots) that will be added to the current airspeed reference. Use negative numbers to subtract.
     */
    int adjustIAS(int addedIAS);

    /**
     * @return The deviation (in degrees) from the ILS glideslope, if available.
     * @note Use `isILSAvailable()` to check if this value is available.
     */
    float getVerticalApproachDeviation();

    /**
     * @return The deviation (in degrees) from the ILS localizer, if available.
     * @note Use `isILSAvailable()` to check if this value is available.
     */
    float getHorizontalApproachDeviation();

    /**
     * @return Whether the ILS is available.
     */
    bool isILSAvailable();

    /**
     * @return The current heading mode.
     */
    APHeadingMode getHeadingMode();

    /**
     * @return The current heading value (in degrees).
     */
    int getHeadingValue();

    /**
     * @return The current altitude mode.
     */
    APHeadingMode getAltitudeMode();

    /**
     * @return The current altitude value (in feet).
     */
    int getAltitudeValue();

    /**
     * @return Whether the auto-throttle is enabled.
     */
    bool getIASMode();

    /**
     * @return The current airspeed reference for the auto-throttle (in knots).
     */
    int getIASValue();

    /**
     * @return Whether the GPWS is enabled.
     */
    bool getGPWSMode();

    /**
     * @return The current vertical speed value (in feet per minute).
     */
    int getVerticalSpeedValue();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
