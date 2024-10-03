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

#include <OgreVector3.h>
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
    * @param filename WAV file.
    * @param resource_group_name Leave empty to auto-search all groups (classic behavior).
    */
    SoundPtr createSound(Ogre::String filename, Ogre::String resource_group_name = "");

    /** Returns the position vector of the listener
     * @return listener position vector
     */
    Ogre::Vector3 getListenerPosition() const { return listener_position; }

    void setListener(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);

    /**
     * Updates the EFX/EAX reverb preset that is used as a base for updating the listener's effect slot.
     * @param listener_environment The preset that will be used for the listener environment.
     * @see updateListenerEffectSlot()
     */
    void setListenerEnvironment(std::string listener_environment);

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
    float getSpeedOfSound() { return alGetFloat(AL_SPEED_OF_SOUND); }

    /**
     * Updates the speed of sound in OpenAL with the provided value.
     * This value should based on RoR units for correct results.
     * @param speed_of_sound Speed of sound within the range of AL_SPEED_OF_SOUND.
     */
    void setSpeedOfSound(float speed_of_sound) { alSpeedOfSound(speed_of_sound); }

    /**
     * @return The value of AL_DOPPLER_FACTOR as currently set in OpenAL.
     */
    float getDopplerFactor() { return alGetFloat(AL_DOPPLER_FACTOR); }

    /**
     * Updates the doppler factor in OpenAL with the provided value.
     * @param doppler_factor Doppler factor within the range of AL_DOPPLER_FACTOR.
     */
    void setDopplerFactor(float doppler_factor) { alDopplerFactor(doppler_factor); }

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
    std::map<std::string, EFXEAXREVERBPROPERTIES> getEfxPropertiesMap() const { return efx_properties_map; }

    static const float MAX_DISTANCE;
    static const float ROLLOFF_FACTOR;
    static const float REFERENCE_DISTANCE;
    static const unsigned int MAX_HARDWARE_SOURCES = 32;
    static const unsigned int MAX_AUDIO_BUFFERS = 8192;

private:
    void recomputeAllSources();

    /**
     * Computes audibility of an audio source and retires it if it is inaudible. Otherwise, it updates
     * its state (e.g. play/stop) if it is already assigned to a hardware source. If it was not assigned
     * to a hardware source yet, it will either be assigned to a free slot or replace a less audible
     * source, if one exists.
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
    int    hardware_sources_num = 0;                       // total number of available hardware sources < MAX_HARDWARE_SOURCES
    int    hardware_sources_in_use_count = 0;
    int    hardware_sources_map[MAX_HARDWARE_SOURCES]; // stores the hardware index for each source. -1 = unmapped
    ALuint hardware_sources[MAX_HARDWARE_SOURCES];     // this buffer contains valid AL handles up to m_hardware_sources_num

    // audio sources
    SoundPtr audio_sources[MAX_AUDIO_BUFFERS] = { nullptr };
    // helper for calculating the most audible sources
    std::pair<int, float> audio_sources_most_audible[MAX_AUDIO_BUFFERS];

    // audio buffers: Array of AL buffers and filenames
    int          audio_buffers_in_use_count = 0;
    ALuint       audio_buffers[MAX_AUDIO_BUFFERS];
    Ogre::String audio_buffer_file_name[MAX_AUDIO_BUFFERS];

    Ogre::Vector3 listener_position = Ogre::Vector3::ZERO;
    Ogre::Vector3 listener_direction = Ogre::Vector3::ZERO;
    Ogre::Vector3 listener_up = Ogre::Vector3::ZERO;
    ALCdevice*    audio_device = nullptr;
    ALCcontext*   sound_context = nullptr;

    // OpenAL EFX stuff
    bool                                            efx_is_available = false;
    ALuint                                          listener_slot = 0;
    ALuint                                          efx_outdoor_obstruction_lowpass_filter_id = 0;
    EfxReverbEngine                                 efx_reverb_engine = EfxReverbEngine::NONE;
    std::string                                     listener_efx_preset_name;
    std::map<std::string, EFXEAXREVERBPROPERTIES>   efx_properties_map;
    std::map<std::string, ALuint>                   efx_effect_id_map;
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
     * @param efx_properties Pointer to a struct holding the parameters of the reverb preset.
     */
    ALuint  CreateAlEffect(const EFXEAXREVERBPROPERTIES* efx_properties);

    /**
     * Helper function that fills the efx_properties_map with presets provided by
     * OpenAL's efx-presets.h header.
     */
    void    prepopulate_efx_property_map();

    /**
     * Dynamically adjusts some parameters of the currently active reverb preset based
     * on the current environment of the listener. It works on the AL effect correspondig
     * to a reverb preset, i.e. the original preset in efx_properties_map remains unchanged.
     * Finally, it updates the AL listener's effect slot with the adjusted preset.
     */
    void    updateListenerEffectSlot();

    /**
     *   Applies an obstruction filter to the provided source if certain conditions apply.
     *   To decide whether the filter should be applied or not, the function performs
     *   various checks against the environment of the listener.
     *   @param hardware_souce The index of the hardware source.
     */
    void    updateObstructionFilter(const ALuint hardware_source);
};

/// @}

} // namespace RoR

#endif // USE_OPENAL
