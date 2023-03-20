
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */

/**
 * @brief Binding of RoR::SoundScriptTemplate; a customizable sound effect.
 * @remark Defined using soundscript file format, see https://docs.rigsofrods.org/vehicle-creation/fileformat-soundscript/.
 */
class SoundScriptTemplateClass
{
public:
    int getNumSounds();
    string getSoundName(int);
    float getSoundPitch(int);

    string name;
    string file_name;
    string group_name;

    /**
    * This template is from the base game, not from a mod.
    */
    bool base_template;
    bool has_start_sound;
    bool has_stop_sound;
    bool unpitchable;

    float gain_multiplier;
    float gain_offset;
    float gain_square;
    ModulationSources gain_source

    float pitch_multiplier;
    float pitch_offset;
    float pitch_square;
    ModulationSources pitch_source

    string start_sound_name;
    float start_sound_pitch;
    string stop_sound_name;
    float stop_sound_pitch;

    SoundTriggers trigger_source;

};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
