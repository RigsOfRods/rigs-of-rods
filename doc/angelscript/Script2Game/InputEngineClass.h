
/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

namespace Script2Game {

/**
 * @brief Binding of RoR::InputEngine; Manages input devices, their configuration (input.map ...) and state.
 * @note This object is created automatically as global variable `inputs`.
 */
class InputEngineClass
{
public:
    /**
     * @return full configuration string for given command, including the EXPL modifier.
     */
    string getEventCommand(inputEvents ev);
    
    /**
     * @return configuration string for given command, omitting the EXPL modifier.
     */
    string getEventCommandTrimmed(inputEvents ev);

    /**
     * @return true if the event input is active (keyboard key pressed etc.).
     */
    bool getEventBoolValue(inputEvents ev);

    /**
     * @return true if the event input is active (keyboard key pressed etc.)
     *         and the bouncing on/off cycle is in 'on' state.
     */
    bool getEventBoolValueBounce(inputEvents ev, float time=0.2);    

};

} //namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
