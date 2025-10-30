/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_OPENAL

#pragma once

#include "Actor.h"
#include "Application.h"
#include "Collisions.h"
#include "GameContext.h"
#include "Sound.h"
#include "SoundScriptManager.h"

#include <Ogre.h>
#include <OgreString.h>

#ifdef __APPLE__
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
  #include <OpenAL/alext.h>
  #include <OpenAL/efx-presets.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
  #include <AL/alext.h>
  #include <AL/efx-presets.h>
#endif // __APPLE__

namespace RoR {

/// @addtogroup Audio
/// @{

class SoundManager
{
    friend class Sound;

public:
    SoundManager();
    ~SoundManager();

    /**
     * Cleans up various objects that should be reset when returning from a terrain to the main menu.
     */
    void     CleanUp();

    /**
    * @param filename WAV file.
    * @param resource_group_name Leave empty to auto-search all groups (classic behavior).
    */
    SoundPtr createSound(Ogre::String filename, Ogre::String resource_group_name = "");

    /** Returns the position vector of the listener
     * @return listener position vector
     */
    Ogre::Vector3 GetListenerPosition() const { return m_listener_position; }

    /**
     * @return True if the listener position is below water level. False otherwise.
     */
    bool ListenerIsUnderwater() const { return m_listener_is_underwater; }

    /**
     * Does the per-frame update of sounds and listener environment. With the help of other functions it
     * determines and then submits the current state of the audio world to OpenAL.
     * @param dt Time since last frame in seconds
     */
    void Update(const float dt);

    /**
     * Updates the global Doppler factor based on CVar settings and the state of the physics simulation.
     */
    void UpdateGlobalDopplerFactor() const;

    /**
     * Updates properties of OpenAL facilities that are only available with EFX.
     * @param dt Time since last frame in seconds
     */
    void UpdateEfxSpecificProperties(const float dt);

    /**
     * Identifies the actor to which the sound corresponding to a hardware source belongs
     * and updates the Doppler factor based on CVar settings and the actor's physics sim state.
     * @param hardware_index Index of the hardware source.
     */
    void UpdateSourceSpecificDopplerFactor(const int hardware_index) const;

    /**
      * Sets position and speed of the listener
      * @param position The position of the listener.
      * @param direction This direction vector specifies where the front of the listener is pointing to.
      * @param up This direction vector specifies where the top of the head of the listener is pointing to.
      * @param velocity The movement speed of the listener in each dimension.
      */
    void SetListener(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);

    /**
     * Unlike the name suggests, this sets the listener's gain to 0, essentially muting all sounds.
     */
    void pauseAllSounds();

    /**
     * Unlike the name suggests, this sets the listener's gain to the value of the CVar audio_master_volume.
     */
    void resumeAllSounds();

    /**
     * Updates both CVar audio_master_volume and the listener's gain to the provided value.
     * @param v Volume within the range of AL_GAIN.
     */
    void setMasterVolume(float v);

    bool isDisabled() { return audio_device == 0; }

    /**
     * @return The value of AL_SPEED_OF_SOUND as currently set in OpenAL.
     */
    float GetSpeedOfSound() const { return alGetFloat(AL_SPEED_OF_SOUND); }

    /**
     * Updates the speed of sound in OpenAL with the provided value.
     * This value should based on RoR units for correct results.
     * @param speed_of_sound Speed of sound within the range of AL_SPEED_OF_SOUND.
     */
    void SetSpeedOfSound(const float speed_of_sound) const { alSpeedOfSound(speed_of_sound); }

    /**
     * @return The value of AL_DOPPLER_FACTOR as currently set in OpenAL.
     */
    float GetDopplerFactor() const { return alGetFloat(AL_DOPPLER_FACTOR); }

    /**
     * Updates the global Doppler factor in OpenAL with the provided value.
     * @param doppler_factor Doppler factor within the range of AL_DOPPLER_FACTOR.
     */
    void SetDopplerFactor(const float doppler_factor) const { alDopplerFactor(doppler_factor); }

    /**
     * Updates the Doppler factor of an OpenAL source with the provided value.
     * EFX has to be supported and enabled, otherwise this is a no-op.
     * @param hardware_source Index of the hardware source.
     * @param doppler_factor Doppler factor within the range of AL_DOPPLER_FACTOR.
     */
     void SetDopplerFactor(const ALuint hardware_source, const float doppler_factor) const;

    /**
     * Sets the air absorptions factor for the direct path of all sounds.
     * @param air_absorption_factor Air absorption factor within the range of AL_AIR_ABSORPTION_FACTOR.
     */
    void SetAirAbsorptionFactor(const float air_absorption_factor) { m_air_absorption_factor = air_absorption_factor; }

    /**
     * @return current value set for the air absorption factor for the direct path of sounds
     */
    float GetAirAbsorptionFactor() const { return m_air_absorption_factor; }

    /**
     * Returns the number of currently used hardware sources. In a typical scenario,
     * this value changes dynamically.
     * @return The number of hardware sources currently in use.
     */
    int getNumHardwareSources() { return hardware_sources_num; }

    /**
    * Returns currently registered EFX presets
    * @return Map of EFX Preset names to their EFXEAXREVERBPROPERTIES object.
    */
    const std::map<std::string, EFXEAXREVERBPROPERTIES>& getEfxPropertiesMap() const { return m_efx_properties_map; }

    /**
     * Returns a pointer to properties of an EFX preset stored in the EFX properties map.
     * The presets should not be modified directly so they can serve as a reference.
     * @param efx_preset_name The name of the preset for which the properties shall be returned.
     * @return Pointer to properties of a reverb preset if the lookup for the name in the EFX Properties map was positive, nullptr otherwise.
     */
    const EFXEAXREVERBPROPERTIES* GetEfxProperties(const std::string& efx_preset_name) const;

    /**
     * Determines which reverb preset corresponds to the provided position and returns its properties.
     * @param position Position for which the reverb preset of the encompassing environment will be returned.
     * @return Reverb properties for the provided position.
     */
    const EFXEAXREVERBPROPERTIES* GetReverbPresetAt(Ogre::Vector3 position) const;

    static const float MAX_DISTANCE;
    static const float ROLLOFF_FACTOR;
    static const float REFERENCE_DISTANCE;
    static const unsigned int MAX_HARDWARE_SOURCES = 32;
    static const unsigned int MAX_AUDIO_BUFFERS = 8192;

private:
    /**
     * Updates the listener's position, orientation and velocity vectors in OpenAL.
     * @see SetListener()
     */
    void UpdateAlListener();

    /**
     * Determines several properties of the environment of the listener and updates OpenAL to use them.
     */
    void UpdateListenerEnvironment();

    void recomputeAllSources();

    /**
     * Computes audibility of an audio source and retires it if it is inaudible. Otherwise, it updates
     * its parameters (e.g. play/stop) if it is already assigned to a hardware source. If it was not assigned
     * to a hardware source yet, it will either be assigned to a free slot or replace a less audible
     * source if one exists.
     * @see assign()
     * @see retire()
     */
    void recomputeSource(int source_index, int reason, float vfl, Ogre::Vector3 *vvec);

    /**
     * Returns the AL handle for the hardware source with the provided index.
     * @param hardware_index Index of the hardware source.
     * @return The AL handle for the requested hardware source.
     */
    ALuint getHardwareSource(int hardware_index) { return hardware_sources[hardware_index]; };

    /**
     * Adds an audio source to hardware source.
     * @param source_index Index of the audio source.
     * @param hardware_index Index of the hardware source to which the audio source will be assigned.
     */
    void assign(int source_index, int hardware_index);

    /**
     * Stops and the removes an audio source from hardware source.
     * @param source_index The index of the audio source.
     */
    void retire(int source_index);

    bool loadWAVFile(Ogre::String filename, ALuint buffer, Ogre::String resource_group_name = "");

    // active audio sources (hardware sources)
    int    hardware_sources_num = 0;                   //!< total number of allocated hardware sources (<= MAX_HARDWARE_SOURCES)
    int    hardware_sources_in_use_count = 0;
    int    hardware_sources_map[MAX_HARDWARE_SOURCES]; //!< maps from the index of a hardware source to the index of the audio source currently assigned to the corresponding hardware source. -1 = unmapped
    ALuint hardware_sources[MAX_HARDWARE_SOURCES];     // this buffer contains valid AL handles up to m_hardware_sources_num

    // audio sources
    int      m_audio_sources_in_use_count = 0;
    SoundPtr audio_sources[MAX_AUDIO_BUFFERS] = { nullptr };
    // helper for calculating the most audible sources
    std::pair<int, float> audio_sources_most_audible[MAX_AUDIO_BUFFERS];

    // audio buffers: Array of AL buffers and filenames
    int          audio_buffers_in_use_count = 0;
    ALuint       audio_buffers[MAX_AUDIO_BUFFERS];
    Ogre::String audio_buffer_file_name[MAX_AUDIO_BUFFERS];

    bool          m_listener_is_underwater = false;
    Ogre::Vector3 m_listener_position = Ogre::Vector3::ZERO;
    Ogre::Vector3 m_listener_direction = Ogre::Vector3::ZERO;
    Ogre::Vector3 m_listener_up = Ogre::Vector3::ZERO;
    Ogre::Vector3 m_listener_velocity = Ogre::Vector3::ZERO;

    ALCdevice*    audio_device = nullptr;
    ALCcontext*   sound_context = nullptr;

    // OpenAL EFX stuff
    bool                                            m_efx_is_available = false;
    ALuint                                          m_listener_slot = 0;
    ALuint                                          m_efx_outdoor_obstruction_lowpass_filter_id = 0;
    ALuint                                          m_efx_occlusion_wet_path_send_id = 0;
    ALuint                                          m_efx_occlusion_wet_path_lowpass_filter_id = 0;
    float                                           m_air_absorption_factor = 1.0f;
    EfxReverbEngine                                 m_efx_reverb_engine = EfxReverbEngine::NONE;
    const EFXEAXREVERBPROPERTIES*                   m_listener_efx_reverb_properties = nullptr;
    std::map<std::string, EFXEAXREVERBPROPERTIES>   m_efx_properties_map;
    std::map<ALuint, ALuint>                        m_efx_effect_id_map;  //<! maps from auxiliary effect slot id to effect id
    LPALGENEFFECTS                                  alGenEffects = nullptr;
    LPALDELETEEFFECTS                               alDeleteEffects = nullptr;
    LPALISEFFECT                                    alIsEffect = nullptr;
    LPALEFFECTI                                     alEffecti = nullptr;
    LPALEFFECTF                                     alEffectf = nullptr;
    LPALEFFECTFV                                    alEffectfv = nullptr;
    LPALGENFILTERS                                  alGenFilters = nullptr;
    LPALDELETEFILTERS                               alDeleteFilters = nullptr;
    LPALISFILTER                                    alIsFilter = nullptr;
    LPALFILTERI                                     alFilteri = nullptr;
    LPALFILTERF                                     alFilterf = nullptr;
    LPALGENAUXILIARYEFFECTSLOTS                     alGenAuxiliaryEffectSlots = nullptr;
    LPALDELETEAUXILIARYEFFECTSLOTS                  alDeleteAuxiliaryEffectSlots = nullptr;
    LPALISAUXILIARYEFFECTSLOT                       alIsAuxiliaryEffectSlot = nullptr;
    LPALAUXILIARYEFFECTSLOTI                        alAuxiliaryEffectSloti = nullptr;
    LPALAUXILIARYEFFECTSLOTF                        alAuxiliaryEffectSlotf = nullptr;
    LPALAUXILIARYEFFECTSLOTFV                       alAuxiliaryEffectSlotfv = nullptr;

    /**
     * Creates an OpenAL effect based on the parameters of an efx/eax reverb preset.
     * @param efx_properties EFXEAXREVERBPROPERTIES object holding the parameters of the reverb preset.
     * @see `AL/efx-presets.h` from OpenAL
     */
    ALuint  CreateAlEffect(const EFXEAXREVERBPROPERTIES* efx_properties) const;

    /**
     * Deletes an OpenAL effect.
     * @param effect_id ID of the effect targeted for removal.
     */
    void    DeleteAlEffect(const ALuint efx_effect_id) const;

    /**
     * Helper function that fills the m_efx_properties_map with presets provided by
     * OpenAL's efx-presets.h header.
     */
    void    PrepopulateEfxPropertiesMap();

    /**
     * Dynamically adjusts some parameters of the currently active reverb preset based
     * on the current environment of the listener. It works on the AL effect correspondig
     * to a reverb preset, i.e. the original preset in m_efx_properties_map remains unchanged.
     * Finally, it updates the AL listener's effect slot with the adjusted preset.
     * @param dt Time since last frame in seconds
     */
    void    UpdateListenerEffectSlot(const float dt);

    /**
     * This performs a smooth update of the efx properties of an OpenAL Auxiliary Effect slot
     * using linear interpolation over several timesteps.
     * @param dt Time since last frame in seconds
     * @param slot_id ID of the AuxiliaryEffectSlot which should be updated
     * @param target_efx_properties EFX Reverb properties to update to
     */
    void    SmoothlyUpdateAlAuxiliaryEffectSlot(const float dt, const ALuint slot_id, const EFXEAXREVERBPROPERTIES* target_efx_properties);

    /**
      * Detects surfaces close to the listener and calculates a user-relative (as opposed to listener-relative)
      * panning vector for early reflections as well as gain and delay values for early reflections based
      * on surface distance.
      * @return A tuple of user-relative panning vector, gain and delay for early reflections
      */
    std::tuple<Ogre::Vector3, float, float> ComputeEarlyReflectionsProperties() const;

    /**
     *   Helper function to call several other functions to update source filters.
     *   @param hardware_index The index of the hardware source.
     */
    void    UpdateSourceFilters(const int hardware_index) const;

    /**
     *   Performs various checks against the environment of the listener to determine
     *   whether the sound belonging to a hardware source is obstructed or not.
     *   @param hardware_index The index of the hardware source.
     *   @return True if the sound is obstructed from the listener's point of view, false otherwise.
     */
    bool    IsHardwareSourceObstructed(const int hardware_index) const;

    /**
     *   Applies an obstruction filter to the provided hardware source.
     *   @param hardware_index The index of the hardware source.
     *   @param enable_obstruction_filter Whether the obstruction filter should be enabled for the hardware source or not.
     */
    void    UpdateObstructionFilter(const int hardware_index, const bool enable_obstruction_filter) const;

    /**
     *   Applies an occlusion filter to the provided source if certain conditions apply.
     *   To decide whether the filter should be applied or not, the function checks
     *   whether the reverb preset of the sound differs from that provided as a parameter.
     *   @param hardware_index The index of the hardware source.
     *   @param effect_slot_id The id of the AL effect slot that will be used as a send target
     *   @param reference_efx_reverb_properties The reverb preset that the reverb preset of the sound location is compared against
     *   @return True if occlusion was detected, false otherwise.
     */
    bool    UpdateOcclusionFilter(const int hardware_index, const ALuint effect_slot_id, const EFXEAXREVERBPROPERTIES* reference_efx_reverb_properties) const;

    /**
     *   Updates AL Cones for sources of directed sound emissions (exhausts, turboprops and turbojets).
     */
    void    UpdateDirectedSounds() const;

    /**
     *    Updates the Cone properties for the hardware source.
     *    @param source The AL source of which the cone properties will be modified.
     *    @param cone_direction The direction the cone will face.
     *    @param cone_inner_angle Angle for the inner zone.
     *    @param cone_outer_angle Angle that marks the border between transitional and outside zone of the cone
     *    @param cone_outer_gain Gain for the outside zone.
     *    @param cone_outer_gain_hf High-frequency gain for the outside zone.
     */
    void    UpdateConeProperties(
                const ALuint          source,
                const Ogre::Vector3&  cone_direction,
                const float           cone_inner_angle,
                const float           cone_outer_angle,
                const float           cone_outer_gain,
                const float           cone_outer_gain_hf
            ) const;
};

/// @}

} // namespace RoR

#endif // USE_OPENAL
