namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Turbojet. Contains information specific to turbojet engines.
 * @note Obtain the object using `BeamClass.getTurbojet(int index)`.
 */
class TurbojetClass
{
    // PLEASE maintain the same order as in 'bindings/TurbojetAngelscript.cpp' and 'physics/air/TurboJet.h'
public:
    /**
     * @return Maximum dry thrust (no afterburner) of the engine, in kilonewtons.
     */
    float getMaxDryThrust();

    /**
     * @return Whether the afterburner is active.
     */
    bool getAfterburner();

    /**
     * @return Maximum thrust with afterburner active (wet thrust), in kilonewtons.
     */
    float getAfterburnerThrust();

    /**
     * @return The exhaust gas velocity, in metres per second.
     */
    float getExhaustVelocity();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
