
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */
 
/**
 * @brief Binding of RoR::Sound; a single sound sample positioned in 3D space (spatial audio).
 * @note To create the object, use `GameScriptClass::createSoundFromResource()`
 */ 
 */
class SoundClass
{
public:
    void setPitch(float pitch);
    void setGain(float gain);
    void setPosition(vector3 pos);
    void setVelocity(vector3 vel);
    void setLoop(bool loop);
    void setEnabled(bool e);
    void play();
    void stop();

    bool getEnabled();
    bool isPlaying();
    float getAudibility();
    float getGain();
    float getPitch();
    bool getLoop();
    
    /** 
    * this value is changed dynamically, depending on whether the input is played or not.
    */
    int getCurrentHardwareIndex();
    uint getBuffer();
    vector3 getPosition();
    vector3 getVelocity();
    
    /**
    * must not be changed during the lifetime of this object.
    */
    int getSourceIndex();
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
