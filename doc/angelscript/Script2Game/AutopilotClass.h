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
     * @brief Returns whether the autopilot is in "custom autopilot mode".
     * @returns `true` if the mode is enabled, `false` otherwise.
     */
    bool getCustomAutopilotMode();

    /**
     * @brief Sets whether the autopilot is in "custom autopilot mode".
     * 
     * When enabled, this mode prevents the default A/P from controlling the aircraft
     * and disables the default logic (such as the A/P disengaging when NAV is engaged
     * and no ILS is available).
     * 
     * You can use this method to replace the default autopilot with your own implementation.
     */
    void setCustomAutopilotMode(bool customAP);

    /**
     * @brief Sets or toggles the heading modes.
     * @param headingMode The heading mode. If the autopilot is in the same mode as `headingMode`, it toggles it. Otherwise, it sets the new mode.
     * @return The new heading mode.
     */
    APHeadingMode toggleHeading(APHeadingMode headingMode);

    /**
     * @brief Sets or toggles the altitude modes.
     * @param altitudeMode The altitude mode. If the autopilot is in the same mode as `altitudeMode`, it toggles it. Otherwise, it sets the new mode.
     * @returns The new altitude mode.
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
     * @param addedIAS The airspeed (in knots) that will be added to the current airspeed reference. Use negative numbers to subtract.
     */
    int adjustIAS(int addedIAS);

    /**
     * @brief Returns the deviation (in degrees) from the ILS glideslope, if available.
     * @returns The deviation from the ILS glideslope.
     * @note Use `isILSAvailable()` to check if this value is available.
     */
    float getVerticalApproachDeviation();

    /**
     * @brief Returns the deviation (in degrees) from the ILS localizer, if available.
     * @return The deviation from the ILS localizer.
     * @note Use `isILSAvailable()` to check if this value is available.
     */
    float getHorizontalApproachDeviation();

    /**
     * @brief Returns whether the ILS is available.
     * @return `true` if the ILS information is available, `false` otherwise.
     */
    bool isILSAvailable();

    /**
     * @brief Gets the current heading mode.
     * @return The current heading mode.
     */
    APHeadingMode getHeadingMode();

    /**
     * @brief Gets the current heading value.
     * @return The current heading value (in degrees).
     */
    int getHeadingValue();

    /**
     * @brief Gets the current altitude mode.
     * @return The current altitude mode.
     */
    APAltitudeMode getAltitudeMode();

    /**
     * @brief Gets the current altitude value.
     * @return The current altitude value (in feet).
     */
    int getAltitudeValue();

    /**
     * @brief Returns whether the auto-throttle is engaged.
     * @return `true` if the auto-throttle is engaged, `false` otherwise.
     */
    bool getIASMode();

    /**
     * @brief Gets the current airspeed reference for the auto-throttle.
     * @return The current airspeed reference for the auto-throttle (in knots).
     */
    int getIASValue();

    /**
     * @brief Returns whether the GPWS is engaged.
     * @return `true` if the GPWS is engaged, `false` otherwise.
     */
    bool getGPWSMode();

    /**
     * @brief Gets the current vertical speed value.
     * @return The current vertical speed value (in feet per minute).
     */
    int getVerticalSpeedValue();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
