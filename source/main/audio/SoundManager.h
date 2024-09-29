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

#include "Application.h"
#include "Collisions.h"
#include "GameContext.h"
#include "Sound.h"

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
    void setListenerEnvironment(std::string listener_environment);
    void pauseAllSounds();
    void resumeAllSounds();
    void setMasterVolume(float v);

    bool isDisabled() { return audio_device == 0; }

    float getSpeedOfSound() { return alGetFloat(AL_SPEED_OF_SOUND); }
    void setSpeedOfSound(float speed_of_sound) { alSpeedOfSound(speed_of_sound); }
    float getDopplerFactor() { return alGetFloat(AL_DOPPLER_FACTOR); }
    void setDopplerFactor(float doppler_factor) { alDopplerFactor(doppler_factor); }

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
    void recomputeSource(int source_index, int reason, float vfl, Ogre::Vector3 *vvec);
    ALuint getHardwareSource(int hardware_index) { return hardware_sources[hardware_index]; };

    void assign(int source_index, int hardware_index);
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

    ALuint  CreateAlEffect(const EFXEAXREVERBPROPERTIES* efx_properties);
    void    prepopulate_efx_property_map();
    void    updateListenerEffectSlot();
};

/// @}

} // namespace RoR

#endif // USE_OPENAL
