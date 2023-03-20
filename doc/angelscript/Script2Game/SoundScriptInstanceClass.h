
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */


/**
 * @brief Binding of RoR::SoundScriptInstance; instance of SoundScriptTemplateClass.
 * @note To create the object, use `GameScriptClass::createSoundScriptInstance()`
 */
class SoundScriptInstanceClass
{
public:
    void runOnce();                         
    void setPitch(float pitch);                
    void setGain(float gain);                  
    void setPosition(vector3 pos);             
    void setVelocity(vector3 velo);            
    void start();                              
    void stop();                               
    void kill();                               

    SoundScriptTemplateClass getTemplate()
    SoundClass getStartSound();            
    SoundClass getStopSound();             
    SoundClass getSound(int pos);          
    float getStartSoundPitchgain();            
    float getStopSoundPitchgain();             
    float getSoundPitchgain(int pos);          
    int getActorInstanceId();                  
    const string& getInstanceName();           
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game