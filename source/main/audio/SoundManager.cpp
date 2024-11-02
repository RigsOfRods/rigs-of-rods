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

#include "SoundManager.h"

#include "Application.h"
#include "Sound.h"

#include <OgreResourceGroupManager.h>

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|Audio] "

bool _checkALErrors(const char* filename, int linenum)
{
    int err = alGetError();
    if (err != AL_NO_ERROR)
    {
        char buf[1000] = {};
        snprintf(buf, 1000, "OpenAL Error: %s (0x%x), @ %s:%d", alGetString(err), err, filename, linenum);
        LOGSTREAM << buf;
        return true;
    }
    return false;
}

#define hasALErrors() _checkALErrors(__FILE__, __LINE__)

using namespace RoR;
using namespace Ogre;

const float SoundManager::MAX_DISTANCE = 500.0f;
const float SoundManager::ROLLOFF_FACTOR = 1.0f;
const float SoundManager::REFERENCE_DISTANCE = 7.5f;

SoundManager::SoundManager()
{
    if (App::audio_device_name->getStr() == "")
    {
        LOGSTREAM << "No audio device configured, opening default.";
        audio_device = alcOpenDevice(nullptr);
    }
    else
    {
        audio_device = alcOpenDevice(App::audio_device_name->getStr().c_str());
        if (!audio_device)
        {
            LOGSTREAM << "Failed to open configured audio device \"" << App::audio_device_name->getStr() << "\", opening default.";
            App::audio_device_name->setStr("");
            audio_device = alcOpenDevice(nullptr);
        }
    }

    if (!audio_device)
    {
        LOGSTREAM << "Failed to open default audio device. Sound disabled.";
        hasALErrors();
        return;
    }

    sound_context = alcCreateContext(audio_device, NULL);

    if (!sound_context)
    {
        alcCloseDevice(audio_device);
        audio_device = NULL;
        hasALErrors();
        return;
    }

    alcMakeContextCurrent(sound_context);

    if (alGetString(AL_VENDOR)) LOG("SoundManager: OpenAL vendor is: " + String(alGetString(AL_VENDOR)));
    if (alGetString(AL_VERSION)) LOG("SoundManager: OpenAL version is: " + String(alGetString(AL_VERSION)));
    if (alGetString(AL_RENDERER)) LOG("SoundManager: OpenAL renderer is: " + String(alGetString(AL_RENDERER)));
    if (alGetString(AL_EXTENSIONS)) LOG("SoundManager: OpenAL extensions are: " + String(alGetString(AL_EXTENSIONS)));
    if (alcGetString(audio_device, ALC_DEVICE_SPECIFIER)) LOG("SoundManager: OpenAL device is: " + String(alcGetString(audio_device, ALC_DEVICE_SPECIFIER)));
    if (alcGetString(audio_device, ALC_EXTENSIONS)) LOG("SoundManager: OpenAL ALC extensions are: " + String(alcGetString(audio_device, ALC_EXTENSIONS)));

    // initialize use of OpenAL EFX extensions
    m_efx_is_available = alcIsExtensionPresent(audio_device, "ALC_EXT_EFX");
    if (m_efx_is_available)
    {
        LOG("SoundManager: Found OpenAL EFX extension");

        // Get OpenAL function pointers
        alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
        alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
        alIsEffect = (LPALISEFFECT)alGetProcAddress("alIsEffect");
        alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
        alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
        alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
        alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
        alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
        alIsFilter = (LPALISFILTER)alGetProcAddress("alIsFilter");
        alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
        alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
        alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
        alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
        alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT)alGetProcAddress("alIsAuxiliaryEffectSlot");
        alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
        alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
        alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV)alGetProcAddress("alAuxiliaryEffectSlotfv");

        if (App::audio_enable_efx->getBool())
        {
            // allow user to change reverb engines at will
            switch(App::audio_efx_reverb_engine->getEnum<EfxReverbEngine>())
            {
                case EfxReverbEngine::EAXREVERB: m_efx_reverb_engine = EfxReverbEngine::EAXREVERB; break;
                case EfxReverbEngine::REVERB:    m_efx_reverb_engine = EfxReverbEngine::REVERB; break;
                default:
                    m_efx_reverb_engine = EfxReverbEngine::NONE;
                    LOG("SoundManager: Reverb engine disabled");
            }

            if(m_efx_reverb_engine == EfxReverbEngine::EAXREVERB)
            {
                if (alGetEnumValue("AL_EFFECT_EAXREVERB") != 0)
                {
                    LOG("SoundManager: OpenAL driver supports AL_EFFECT_EAXREVERB, using it");
                }
                else
                {
                    LOG("SoundManager: AL_EFFECT_EAXREVERB requested but OpenAL driver does not support it, falling back to standard reverb. Advanced features, such as reflection panning, will not be available");
                    m_efx_reverb_engine = EfxReverbEngine::REVERB;
                }
            }
            else if(m_efx_reverb_engine == EfxReverbEngine::REVERB)
            {
                LOG("SoundManager: Using OpenAL standard reverb");
            }

            // create effect slot for the listener
            if(!this->alIsAuxiliaryEffectSlot(m_listener_slot))
            {
                alGetError();

                this->alGenAuxiliaryEffectSlots(1, &m_listener_slot);
                ALuint error = alGetError();

                if (error != AL_NO_ERROR)
                {
                    LOG("SoundManager: alGenAuxiliaryEffectSlots for listener_slot failed: " + TOSTRING(alGetString(error)));
                    m_listener_slot = AL_EFFECTSLOT_NULL;
                }
            }

            this->PrepopulateEfxPropertiesMap();

            /*
                Create filter for obstruction
                Currently we don't check for how much high-frequency content the obstacle
                lets through. We assume it's a hard surface with significant absorption
                of high frequencies (which should be true for trucks, buildings and terrain).
            */
            alGetError();

            this->alGenFilters(1, &m_efx_outdoor_obstruction_lowpass_filter_id);
            ALuint e = alGetError();

            if (e != AL_NO_ERROR)
            {
                m_efx_outdoor_obstruction_lowpass_filter_id = AL_FILTER_NULL;
            }
            else
            {
                this->alFilteri(m_efx_outdoor_obstruction_lowpass_filter_id, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
                this->alFilterf(m_efx_outdoor_obstruction_lowpass_filter_id, AL_LOWPASS_GAIN, 0.33f);
                this->alFilterf(m_efx_outdoor_obstruction_lowpass_filter_id, AL_LOWPASS_GAINHF, 0.25f);
            }
        }
    }
    else
    {
        LOG("SoundManager: OpenAL EFX extension not found, disabling EFX");
        App::audio_enable_efx->setVal(false);
    }

    // generate the AL sources
    for (hardware_sources_num = 0; hardware_sources_num < MAX_HARDWARE_SOURCES; hardware_sources_num++)
    {
        alGetError();
        alGenSources(1, &hardware_sources[hardware_sources_num]);
        if (alGetError() != AL_NO_ERROR)
            break;
        alSourcef(hardware_sources[hardware_sources_num], AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE);
        alSourcef(hardware_sources[hardware_sources_num], AL_ROLLOFF_FACTOR, ROLLOFF_FACTOR);
        alSourcef(hardware_sources[hardware_sources_num], AL_MAX_DISTANCE, MAX_DISTANCE);

        // connect source to listener slot effect
        if(App::audio_enable_efx->getBool())
        {
            alSource3i(hardware_sources[hardware_sources_num], AL_AUXILIARY_SEND_FILTER, m_listener_slot, 0, AL_FILTER_NULL);
        }
    }

    alDopplerFactor(App::audio_doppler_factor->getFloat());
    alSpeedOfSound(343.3f);

    for (int i = 0; i < MAX_HARDWARE_SOURCES; i++)
    {
        hardware_sources_map[i] = -1;
    }


}

SoundManager::~SoundManager()
{
    // delete the sources and buffers
    alDeleteSources(MAX_HARDWARE_SOURCES, hardware_sources);
    alDeleteBuffers(MAX_AUDIO_BUFFERS, audio_buffers);

    if(m_efx_is_available)
    {
        if(this->alIsFilter(m_efx_outdoor_obstruction_lowpass_filter_id))
        {
            this->alDeleteFilters(1, &m_efx_outdoor_obstruction_lowpass_filter_id);
        }

        if (this->alIsAuxiliaryEffectSlot(m_listener_slot))
        {
            this->alAuxiliaryEffectSloti(m_listener_slot, AL_EFFECTSLOT_EFFECT, AL_EFFECTSLOT_NULL);
            this->alDeleteAuxiliaryEffectSlots(1, &m_listener_slot);
            m_listener_slot = 0;
        }
    }

    CleanUp();

    // destroy the sound context and device
    sound_context = alcGetCurrentContext();
    audio_device = alcGetContextsDevice(sound_context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(sound_context);
    if (audio_device)
    {
        alcCloseDevice(audio_device);
    }
    LOG("SoundManager destroyed.");
}

void SoundManager::CleanUp()
{
    if(m_efx_is_available)
    {
        m_listener_efx_reverb_properties = nullptr;
        if (this->alIsAuxiliaryEffectSlot(m_listener_slot))
        {
            this->alAuxiliaryEffectSloti(m_listener_slot, AL_EFFECTSLOT_EFFECT, AL_EFFECTSLOT_NULL);
        }

        for (auto it = m_efx_effect_id_map.begin(); it != m_efx_effect_id_map.end();)
        {
            this->DeleteAlEffect(it->second);
            it = m_efx_effect_id_map.erase(it);
        }
    }

    // TODO: Delete Sounds and buffers
}

const EFXEAXREVERBPROPERTIES* SoundManager::GetEfxProperties(const std::string& efx_preset_name) const
{
    const auto it = m_efx_properties_map.find(efx_preset_name);

    if (it != m_efx_properties_map.end())
    {
        return &it->second;
    }
    else
    {
        return nullptr;
    }
}

void SoundManager::PrepopulateEfxPropertiesMap()
{
    m_efx_properties_map["EFX_REVERB_PRESET_GENERIC"]                   = EFX_REVERB_PRESET_GENERIC;
    m_efx_properties_map["EFX_REVERB_PRESET_ROOM"]                      = EFX_REVERB_PRESET_ROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_CAVE"]                      = EFX_REVERB_PRESET_CAVE;
    m_efx_properties_map["EFX_REVERB_PRESET_ARENA"]                     = EFX_REVERB_PRESET_ARENA;
    m_efx_properties_map["EFX_REVERB_PRESET_HANGAR"]                    = EFX_REVERB_PRESET_HANGAR;
    m_efx_properties_map["EFX_REVERB_PRESET_ALLEY"]                     = EFX_REVERB_PRESET_ALLEY;
    m_efx_properties_map["EFX_REVERB_PRESET_HALLWAY"]                   = EFX_REVERB_PRESET_HALLWAY;
    m_efx_properties_map["EFX_REVERB_PRESET_FOREST"]                    = EFX_REVERB_PRESET_FOREST;
    m_efx_properties_map["EFX_REVERB_PRESET_CITY"]                      = EFX_REVERB_PRESET_CITY;
    m_efx_properties_map["EFX_REVERB_PRESET_MOUNTAINS"]                 = EFX_REVERB_PRESET_MOUNTAINS;
    m_efx_properties_map["EFX_REVERB_PRESET_QUARRY"]                    = EFX_REVERB_PRESET_QUARRY;
    m_efx_properties_map["EFX_REVERB_PRESET_PLAIN"]                     = EFX_REVERB_PRESET_PLAIN;
    m_efx_properties_map["EFX_REVERB_PRESET_PARKINGLOT"]                = EFX_REVERB_PRESET_PARKINGLOT;
    m_efx_properties_map["EFX_REVERB_PRESET_UNDERWATER"]                = EFX_REVERB_PRESET_UNDERWATER;
    m_efx_properties_map["EFX_REVERB_PRESET_DRUGGED"]                   = EFX_REVERB_PRESET_DRUGGED;
    m_efx_properties_map["EFX_REVERB_PRESET_DIZZY"]                     = EFX_REVERB_PRESET_DIZZY;
    m_efx_properties_map["EFX_REVERB_PRESET_CASTLE_SHORTPASSAGE"]       = EFX_REVERB_PRESET_CASTLE_SHORTPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_CASTLE_LARGEROOM"]          = EFX_REVERB_PRESET_CASTLE_LARGEROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_CASTLE_LONGPASSAGE"]        = EFX_REVERB_PRESET_CASTLE_LONGPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_CASTLE_HALL"]               = EFX_REVERB_PRESET_CASTLE_HALL;
    m_efx_properties_map["EFX_REVERB_PRESET_CASTLE_COURTYARD"]          = EFX_REVERB_PRESET_CASTLE_COURTYARD;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_SMALLROOM"]         = EFX_REVERB_PRESET_FACTORY_SMALLROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_SHORTPASSAGE"]      = EFX_REVERB_PRESET_FACTORY_SHORTPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_MEDIUMROOM"]        = EFX_REVERB_PRESET_FACTORY_MEDIUMROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_LARGEROOM"]         = EFX_REVERB_PRESET_FACTORY_LARGEROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_LONGPASSAGE"]       = EFX_REVERB_PRESET_FACTORY_LONGPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_HALL"]              = EFX_REVERB_PRESET_FACTORY_HALL;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_COURTYARD"]         = EFX_REVERB_PRESET_FACTORY_COURTYARD;
    m_efx_properties_map["EFX_REVERB_PRESET_FACTORY_ALCOVE"]            = EFX_REVERB_PRESET_FACTORY_ALCOVE;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_SMALLROOM"]    = EFX_REVERB_PRESET_SPACESTATION_SMALLROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_SHORTPASSAGE"] = EFX_REVERB_PRESET_SPACESTATION_SHORTPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_MEDIUMROOM"]   = EFX_REVERB_PRESET_SPACESTATION_MEDIUMROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_LARGEROOM"]    = EFX_REVERB_PRESET_SPACESTATION_LARGEROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_LONGPASSAGE"]  = EFX_REVERB_PRESET_SPACESTATION_LONGPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_SPACESTATION_HALL"]         = EFX_REVERB_PRESET_SPACESTATION_HALL;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_SMALLROOM"]          = EFX_REVERB_PRESET_WOODEN_SMALLROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_SHORTPASSAGE"]       = EFX_REVERB_PRESET_WOODEN_SHORTPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_MEDIUMROOM"]         = EFX_REVERB_PRESET_WOODEN_MEDIUMROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_LARGEROOM"]          = EFX_REVERB_PRESET_WOODEN_LARGEROOM;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_LONGPASSAGE"]        = EFX_REVERB_PRESET_WOODEN_LONGPASSAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_HALL"]               = EFX_REVERB_PRESET_WOODEN_HALL;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_COURTYARD"]          = EFX_REVERB_PRESET_WOODEN_COURTYARD;
    m_efx_properties_map["EFX_REVERB_PRESET_WOODEN_ALCOVE"]             = EFX_REVERB_PRESET_WOODEN_ALCOVE;
    m_efx_properties_map["EFX_REVERB_PRESET_SPORT_EMPTYSTADIUM"]        = EFX_REVERB_PRESET_SPORT_EMPTYSTADIUM;
    m_efx_properties_map["EFX_REVERB_PRESET_SPORT_FULLSTADIUM"]         = EFX_REVERB_PRESET_SPORT_FULLSTADIUM;
    m_efx_properties_map["EFX_REVERB_PRESET_SPORT_STADIUMTANNOY"]       = EFX_REVERB_PRESET_SPORT_STADIUMTANNOY;
    m_efx_properties_map["EFX_REVERB_PRESET_PREFAB_WORKSHOP"]           = EFX_REVERB_PRESET_PREFAB_WORKSHOP;
    m_efx_properties_map["EFX_REVERB_PRESET_PREFAB_OUTHOUSE"]           = EFX_REVERB_PRESET_PREFAB_OUTHOUSE;
    m_efx_properties_map["EFX_REVERB_PRESET_PREFAB_CARAVAN"]            = EFX_REVERB_PRESET_PREFAB_CARAVAN;
    m_efx_properties_map["EFX_REVERB_PRESET_PIPE_LARGE"]                = EFX_REVERB_PRESET_PIPE_LARGE;
    m_efx_properties_map["EFX_REVERB_PRESET_PIPE_LONGTHIN"]             = EFX_REVERB_PRESET_PIPE_LONGTHIN;
    m_efx_properties_map["EFX_REVERB_PRESET_PIPE_RESONANT"]             = EFX_REVERB_PRESET_PIPE_RESONANT;
    m_efx_properties_map["EFX_REVERB_PRESET_OUTDOORS_BACKYARD"]         = EFX_REVERB_PRESET_OUTDOORS_BACKYARD;
    m_efx_properties_map["EFX_REVERB_PRESET_OUTDOORS_ROLLINGPLAINS"]    = EFX_REVERB_PRESET_OUTDOORS_ROLLINGPLAINS;
    m_efx_properties_map["EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON"]       = EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON;
    m_efx_properties_map["EFX_REVERB_PRESET_OUTDOORS_CREEK"]            = EFX_REVERB_PRESET_OUTDOORS_CREEK;
    m_efx_properties_map["EFX_REVERB_PRESET_OUTDOORS_VALLEY"]           = EFX_REVERB_PRESET_OUTDOORS_VALLEY;
    m_efx_properties_map["EFX_REVERB_PRESET_MOOD_HEAVEN"]               = EFX_REVERB_PRESET_MOOD_HEAVEN;
    m_efx_properties_map["EFX_REVERB_PRESET_MOOD_HELL"]                 = EFX_REVERB_PRESET_MOOD_HELL;
    m_efx_properties_map["EFX_REVERB_PRESET_MOOD_MEMORY"]               = EFX_REVERB_PRESET_MOOD_MEMORY;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_COMMENTATOR"]       = EFX_REVERB_PRESET_DRIVING_COMMENTATOR;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_PITGARAGE"]         = EFX_REVERB_PRESET_DRIVING_PITGARAGE;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_INCAR_RACER"]       = EFX_REVERB_PRESET_DRIVING_INCAR_RACER;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_INCAR_SPORTS"]      = EFX_REVERB_PRESET_DRIVING_INCAR_SPORTS;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_INCAR_LUXURY"]      = EFX_REVERB_PRESET_DRIVING_INCAR_LUXURY;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_FULLGRANDSTAND"]    = EFX_REVERB_PRESET_DRIVING_FULLGRANDSTAND;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_EMPTYGRANDSTAND"]   = EFX_REVERB_PRESET_DRIVING_EMPTYGRANDSTAND;
    m_efx_properties_map["EFX_REVERB_PRESET_DRIVING_TUNNEL"]            = EFX_REVERB_PRESET_DRIVING_TUNNEL;
    m_efx_properties_map["EFX_REVERB_PRESET_CITY_STREETS"]              = EFX_REVERB_PRESET_CITY_STREETS;
    m_efx_properties_map["EFX_REVERB_PRESET_CITY_SUBWAY"]               = EFX_REVERB_PRESET_CITY_SUBWAY;
    m_efx_properties_map["EFX_REVERB_PRESET_CITY_UNDERPASS"]            = EFX_REVERB_PRESET_CITY_UNDERPASS;
    m_efx_properties_map["EFX_REVERB_PRESET_CITY_ABANDONED"]            = EFX_REVERB_PRESET_CITY_ABANDONED;
}

void SoundManager::Update(const float dt_sec)
{
    if (!audio_device)
        return;

    recomputeAllSources();
    UpdateAlListener();

    if(App::audio_enable_efx->getBool())
    {
        // apply filters to sources when appropriate
        for(int hardware_index = 0; hardware_index < hardware_sources_num; hardware_index++)
        {
            // update air absorption factor
            alSourcef(hardware_sources[hardware_index], AL_AIR_ABSORPTION_FACTOR, m_air_absorption_factor);

            this->UpdateObstructionFilter(hardware_index);
        }

        this->UpdateListenerEffectSlot(dt_sec);
    }
}

void SoundManager::SetListener(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity)
{
    m_listener_position = position;
    m_listener_direction = direction;
    m_listener_up = up;
    m_listener_velocity = velocity;
}

void SoundManager::UpdateAlListener()
{
    float orientation[6];
    // direction
    orientation[0] = m_listener_direction.x;
    orientation[1] = m_listener_direction.y;
    orientation[2] = m_listener_direction.z;
    // up
    orientation[3] = m_listener_up.x;
    orientation[4] = m_listener_up.y;
    orientation[5] = m_listener_up.z;

    alListener3f(AL_POSITION, m_listener_position.x, m_listener_position.y, m_listener_position.z);
    alListener3f(AL_VELOCITY, m_listener_velocity.x, m_listener_velocity.y, m_listener_velocity.z);
    alListenerfv(AL_ORIENTATION, orientation);
}

void SoundManager::SetListenerEnvironment(const EFXEAXREVERBPROPERTIES* listener_reverb_properties)
{
    m_listener_efx_reverb_properties = listener_reverb_properties;
}

void SoundManager::UpdateListenerEffectSlot(const float dt_sec)
{
    if(m_listener_efx_reverb_properties == nullptr)
    {
        this->SmoothlyUpdateAlAuxiliaryEffectSlot(dt_sec, m_listener_slot, nullptr);
        return;
    }

    EFXEAXREVERBPROPERTIES current_environmental_properties = *m_listener_efx_reverb_properties;

    // early reflections panning, delay and strength
    if (App::audio_enable_reflection_panning->getBool() && m_efx_reverb_engine == EfxReverbEngine::EAXREVERB)
    {
        std::tuple<Ogre::Vector3, float, float> target_early_reflections_properties = this->ComputeEarlyReflectionsProperties();

        // convert panning vector from RHS to EAXREVERB's LHS
        current_environmental_properties.flReflectionsPan[0] =  std::get<0>(target_early_reflections_properties).x;
        current_environmental_properties.flReflectionsPan[1] =  0;
        current_environmental_properties.flReflectionsPan[2] = -std::get<0>(target_early_reflections_properties).z;

        current_environmental_properties.flReflectionsGain   =  std::get<1>(target_early_reflections_properties);
        current_environmental_properties.flReflectionsDelay  =  std::get<2>(target_early_reflections_properties);
    }

    this->SmoothlyUpdateAlAuxiliaryEffectSlot(dt_sec, m_listener_slot, &current_environmental_properties);
}

void SoundManager::SmoothlyUpdateAlAuxiliaryEffectSlot(const float dt_sec, const ALuint slot_id, const EFXEAXREVERBPROPERTIES* target_efx_properties)
{
    const float time_to_target = 0.333f; // seconds to reach the target properties from the current properties
    const float step = std::min(dt_sec / time_to_target, 1.0f);
    static std::map<ALuint, EFXEAXREVERBPROPERTIES> current_efx_properties_of_slot;

    if (target_efx_properties == nullptr)
    {
        this->alAuxiliaryEffectSloti(slot_id, AL_EFFECTSLOT_EFFECT, AL_EFFECTSLOT_NULL);
        return;
    }

    const auto it = current_efx_properties_of_slot.find(slot_id);
    if (it == current_efx_properties_of_slot.end())
    {
        // previously unseen effect slot, set a starting point
        current_efx_properties_of_slot[slot_id] = *target_efx_properties;
    }

    ALuint efx_effect_id;
    // create new AL effect if not existing
    if(m_efx_effect_id_map.find(slot_id) == m_efx_effect_id_map.end())
    {
        efx_effect_id = this->CreateAlEffect(target_efx_properties);
        m_efx_effect_id_map[slot_id] = efx_effect_id;
    }
    else
    {
        efx_effect_id = m_efx_effect_id_map.find(slot_id)->second;
    }

    // compute intermediate step between current and target properties using linear interpolation based on time step
    current_efx_properties_of_slot[slot_id] =
    {
        current_efx_properties_of_slot[slot_id].flDensity                      + step * (target_efx_properties->flDensity             - current_efx_properties_of_slot[slot_id].flDensity),
        current_efx_properties_of_slot[slot_id].flDiffusion                    + step * (target_efx_properties->flDiffusion           - current_efx_properties_of_slot[slot_id].flDiffusion),
        current_efx_properties_of_slot[slot_id].flGain                         + step * (target_efx_properties->flGain                - current_efx_properties_of_slot[slot_id].flGain),
        current_efx_properties_of_slot[slot_id].flGainHF                       + step * (target_efx_properties->flGainHF              - current_efx_properties_of_slot[slot_id].flGainHF),
        current_efx_properties_of_slot[slot_id].flGainLF                       + step * (target_efx_properties->flGainLF              - current_efx_properties_of_slot[slot_id].flGainLF),
        current_efx_properties_of_slot[slot_id].flDecayTime                    + step * (target_efx_properties->flDecayTime           - current_efx_properties_of_slot[slot_id].flDecayTime),
        current_efx_properties_of_slot[slot_id].flDecayHFRatio                 + step * (target_efx_properties->flDecayHFRatio        - current_efx_properties_of_slot[slot_id].flDecayHFRatio),
        current_efx_properties_of_slot[slot_id].flDecayLFRatio                 + step * (target_efx_properties->flDecayLFRatio        - current_efx_properties_of_slot[slot_id].flDecayLFRatio),
        current_efx_properties_of_slot[slot_id].flReflectionsGain              + step * (target_efx_properties->flReflectionsGain     - current_efx_properties_of_slot[slot_id].flReflectionsGain),
        current_efx_properties_of_slot[slot_id].flReflectionsDelay             + step * (target_efx_properties->flReflectionsDelay    - current_efx_properties_of_slot[slot_id].flReflectionsDelay),
        current_efx_properties_of_slot[slot_id].flReflectionsPan[0]            + step * (target_efx_properties->flReflectionsPan[0]   - current_efx_properties_of_slot[slot_id].flReflectionsPan[0]),
        current_efx_properties_of_slot[slot_id].flReflectionsPan[1]            + step * (target_efx_properties->flReflectionsPan[1]   - current_efx_properties_of_slot[slot_id].flReflectionsPan[1]),
        current_efx_properties_of_slot[slot_id].flReflectionsPan[2]            + step * (target_efx_properties->flReflectionsPan[2]   - current_efx_properties_of_slot[slot_id].flReflectionsPan[2]),
        current_efx_properties_of_slot[slot_id].flLateReverbGain               + step * (target_efx_properties->flLateReverbGain      - current_efx_properties_of_slot[slot_id].flLateReverbGain),
        current_efx_properties_of_slot[slot_id].flLateReverbDelay              + step * (target_efx_properties->flLateReverbDelay     - current_efx_properties_of_slot[slot_id].flLateReverbDelay),
        current_efx_properties_of_slot[slot_id].flLateReverbPan[0]             + step * (target_efx_properties->flLateReverbPan[0]    - current_efx_properties_of_slot[slot_id].flLateReverbPan[0]),
        current_efx_properties_of_slot[slot_id].flLateReverbPan[1]             + step * (target_efx_properties->flLateReverbPan[1]    - current_efx_properties_of_slot[slot_id].flLateReverbPan[1]),
        current_efx_properties_of_slot[slot_id].flLateReverbPan[2]             + step * (target_efx_properties->flLateReverbPan[2]    - current_efx_properties_of_slot[slot_id].flLateReverbPan[2]),
        current_efx_properties_of_slot[slot_id].flEchoTime                     + step * (target_efx_properties->flEchoTime            - current_efx_properties_of_slot[slot_id].flEchoTime),
        current_efx_properties_of_slot[slot_id].flEchoDepth                    + step * (target_efx_properties->flEchoDepth           - current_efx_properties_of_slot[slot_id].flEchoDepth),
        current_efx_properties_of_slot[slot_id].flModulationTime               + step * (target_efx_properties->flModulationTime      - current_efx_properties_of_slot[slot_id].flModulationTime),
        current_efx_properties_of_slot[slot_id].flModulationDepth              + step * (target_efx_properties->flModulationDepth     - current_efx_properties_of_slot[slot_id].flModulationDepth),
        current_efx_properties_of_slot[slot_id].flAirAbsorptionGainHF          + step * (target_efx_properties->flAirAbsorptionGainHF - current_efx_properties_of_slot[slot_id].flAirAbsorptionGainHF),
        current_efx_properties_of_slot[slot_id].flHFReference                  + step * (target_efx_properties->flHFReference         - current_efx_properties_of_slot[slot_id].flHFReference),
        current_efx_properties_of_slot[slot_id].flLFReference                  + step * (target_efx_properties->flLFReference         - current_efx_properties_of_slot[slot_id].flLFReference),
        current_efx_properties_of_slot[slot_id].flRoomRolloffFactor            + step * (target_efx_properties->flRoomRolloffFactor   - current_efx_properties_of_slot[slot_id].flRoomRolloffFactor),
        static_cast<int>(std::round(current_efx_properties_of_slot[slot_id].iDecayHFLimit + step * (target_efx_properties->iDecayHFLimit - current_efx_properties_of_slot[slot_id].iDecayHFLimit))),
    };

    // update AL effect to intermediate values
    switch (m_efx_reverb_engine)
    {
        case EfxReverbEngine::EAXREVERB:
            this->alEffectf( efx_effect_id, AL_EAXREVERB_DENSITY,               current_efx_properties_of_slot[slot_id].flDensity);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_DIFFUSION,             current_efx_properties_of_slot[slot_id].flDiffusion);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_GAIN,                  current_efx_properties_of_slot[slot_id].flGain);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_GAINHF,                current_efx_properties_of_slot[slot_id].flGainHF);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_GAINLF,                current_efx_properties_of_slot[slot_id].flGainLF);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_DECAY_TIME,            current_efx_properties_of_slot[slot_id].flDecayTime);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_DECAY_HFRATIO,         current_efx_properties_of_slot[slot_id].flDecayHFRatio);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_DECAY_LFRATIO,         current_efx_properties_of_slot[slot_id].flDecayLFRatio);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_REFLECTIONS_GAIN,      current_efx_properties_of_slot[slot_id].flReflectionsGain);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_REFLECTIONS_DELAY,     current_efx_properties_of_slot[slot_id].flReflectionsDelay);
            this->alEffectfv(efx_effect_id, AL_EAXREVERB_REFLECTIONS_PAN,       current_efx_properties_of_slot[slot_id].flReflectionsPan);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_LATE_REVERB_GAIN,      current_efx_properties_of_slot[slot_id].flLateReverbGain);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_LATE_REVERB_DELAY,     current_efx_properties_of_slot[slot_id].flLateReverbDelay);
            this->alEffectfv(efx_effect_id, AL_EAXREVERB_LATE_REVERB_PAN,       current_efx_properties_of_slot[slot_id].flLateReverbPan);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_ECHO_TIME,             current_efx_properties_of_slot[slot_id].flEchoTime);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_ECHO_DEPTH,            current_efx_properties_of_slot[slot_id].flEchoDepth);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_MODULATION_TIME,       current_efx_properties_of_slot[slot_id].flModulationTime);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_MODULATION_DEPTH,      current_efx_properties_of_slot[slot_id].flModulationDepth);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, current_efx_properties_of_slot[slot_id].flAirAbsorptionGainHF);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_HFREFERENCE,           current_efx_properties_of_slot[slot_id].flHFReference);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_LFREFERENCE,           current_efx_properties_of_slot[slot_id].flLFReference);
            this->alEffectf( efx_effect_id, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR,   current_efx_properties_of_slot[slot_id].flRoomRolloffFactor);
            this->alEffecti( efx_effect_id, AL_EAXREVERB_DECAY_HFLIMIT,         current_efx_properties_of_slot[slot_id].iDecayHFLimit);
            break;

        case EfxReverbEngine::REVERB:
            this->alEffectf( efx_effect_id, AL_REVERB_DENSITY,               current_efx_properties_of_slot[slot_id].flDensity);
            this->alEffectf( efx_effect_id, AL_REVERB_DIFFUSION,             current_efx_properties_of_slot[slot_id].flDiffusion);
            this->alEffectf( efx_effect_id, AL_REVERB_GAIN,                  current_efx_properties_of_slot[slot_id].flGain);
            this->alEffectf( efx_effect_id, AL_REVERB_GAINHF,                current_efx_properties_of_slot[slot_id].flGainHF);
            this->alEffectf( efx_effect_id, AL_REVERB_DECAY_TIME,            current_efx_properties_of_slot[slot_id].flDecayTime);
            this->alEffectf( efx_effect_id, AL_REVERB_DECAY_HFRATIO,         current_efx_properties_of_slot[slot_id].flDecayHFRatio);
            this->alEffectf( efx_effect_id, AL_REVERB_REFLECTIONS_GAIN,      current_efx_properties_of_slot[slot_id].flReflectionsGain);
            this->alEffectf( efx_effect_id, AL_REVERB_REFLECTIONS_DELAY,     current_efx_properties_of_slot[slot_id].flReflectionsDelay);
            this->alEffectf( efx_effect_id, AL_REVERB_LATE_REVERB_GAIN,      current_efx_properties_of_slot[slot_id].flLateReverbGain);
            this->alEffectf( efx_effect_id, AL_REVERB_LATE_REVERB_DELAY,     current_efx_properties_of_slot[slot_id].flLateReverbDelay);
            this->alEffectf( efx_effect_id, AL_REVERB_AIR_ABSORPTION_GAINHF, current_efx_properties_of_slot[slot_id].flAirAbsorptionGainHF);
            this->alEffectf( efx_effect_id, AL_REVERB_ROOM_ROLLOFF_FACTOR,   current_efx_properties_of_slot[slot_id].flRoomRolloffFactor);
            this->alEffectf( efx_effect_id, AL_REVERB_DECAY_HFLIMIT,         current_efx_properties_of_slot[slot_id].iDecayHFLimit);
            break;

        case EfxReverbEngine::NONE:
            this->alAuxiliaryEffectSloti(slot_id, AL_EFFECTSLOT_EFFECT, AL_EFFECTSLOT_NULL);
            return;
    }

    // make the slot use the updated AL effect
    this->alAuxiliaryEffectSloti(slot_id, AL_EFFECTSLOT_EFFECT, efx_effect_id);
}

std::tuple<Ogre::Vector3, float, float> SoundManager::ComputeEarlyReflectionsProperties() const
{
    const float     max_distance = 2.0f;
    const float     reflections_gain_boost_max = 2.0f; // 6.32 db
    float           early_reflections_gain;
    float           early_reflections_delay;
    float           magnitude = 0;
    Ogre::Vector3   early_reflections_pan = { 0.0f, 0.0f, 0.0f};

    /*
     * To detect surfaces around the listener within the vicinity of
     * max_distance, we cast rays counter-clockwise in a 360° circle
     * around the listener on a horizontal plane realative to the listener.
     */
    bool        nearby_surface_detected = false;
    const float angle_step_size = 90;
    float       closest_surface_distance = std::numeric_limits<float>::max();

    for (float angle = 0; angle < 360; angle += angle_step_size)
    {
        float closest_surface_distance_in_this_direction = std::numeric_limits<float>::max();
        Ogre::Vector3 raycast_direction = Quaternion(Ogre::Degree(angle), m_listener_up) * m_listener_direction;
        raycast_direction.normalise();
        // accompany direction vector for how the intersectsTris function works

        // check for nearby collision meshes
        Ray ray = Ray(m_listener_position, raycast_direction * max_distance * App::GetGameContext()->GetTerrain()->GetCollisions()->GetCellSize());
        std::pair<bool, Ogre::Real> intersection = App::GetGameContext()->GetTerrain()->GetCollisions()->intersectsTris(ray);

        if (intersection.first)
        {
            closest_surface_distance_in_this_direction = intersection.second * max_distance;
        }

        ray.setDirection(ray.getDirection().normalisedCopy());

        // check for nearby collision boxes
        for (const collision_box_t& collision_box : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionBoxes())
        {
            if (!collision_box.enabled || collision_box.virt) { continue; }
            intersection = ray.intersects(Ogre::AxisAlignedBox(collision_box.lo, collision_box.hi));
            if (intersection.first && intersection.second <= max_distance)
            {
                closest_surface_distance_in_this_direction = std::min(closest_surface_distance_in_this_direction, intersection.second);
            }
        }

        // check for nearby actors
        const ActorPtrVec& actors = App::GetGameContext()->GetActorManager()->GetActors();
        for(const ActorPtr& actor : actors)
        {
            // ignore own truck if player is driving one
            if (actor == App::GetGameContext()->GetPlayerCharacter()->GetActorCoupling()) { continue; }

            intersection = ray.intersects(actor->ar_bounding_box);
            if (intersection.first && intersection.second <= max_distance)
            {
                closest_surface_distance_in_this_direction = std::min(closest_surface_distance_in_this_direction, intersection.second);
            }
        }

        closest_surface_distance = std::min(closest_surface_distance, closest_surface_distance_in_this_direction);

        if(closest_surface_distance_in_this_direction <= max_distance)
        {
            early_reflections_pan += raycast_direction * (max_distance - closest_surface_distance_in_this_direction);
        }
    }

    nearby_surface_detected = closest_surface_distance <= max_distance;

    // TODO vertical raycasts

    if (!nearby_surface_detected)
    {
        // reset values to the original values of the preset
        early_reflections_delay = m_listener_efx_reverb_properties->flReflectionsDelay;
        early_reflections_gain  = m_listener_efx_reverb_properties->flReflectionsGain;
    }
    else // at least one nearby surface was detected
    {
        // we assume that surfaces further away cause less focussed reflections
        magnitude               = 1.0f - early_reflections_pan.length() / Ogre::Math::Sqrt(2.0f * Ogre::Math::Pow(max_distance, 2));

        // set delay based on distance to the closest surface
        early_reflections_delay = closest_surface_distance / GetSpeedOfSound();

        early_reflections_gain  = std::min(
            (m_listener_efx_reverb_properties->flReflectionsGain
               + reflections_gain_boost_max
               - (reflections_gain_boost_max * (magnitude))),
             AL_EAXREVERB_MAX_REFLECTIONS_GAIN);
    }

    // transform the pan vector from being listener-relative to being user-relative

    // determine the rotation of the listener direction from straight-ahead vector
    // work around Quaternion quirks at around 180° rotation
    Ogre::Quaternion horizontal_rotation;
    if (m_listener_direction.z > 0.0f)
    {
        horizontal_rotation = Quaternion(Ogre::Degree(180), m_listener_up) * m_listener_direction.getRotationTo(Ogre::Vector3::UNIT_Z);
    }
    else
    {
        horizontal_rotation =  m_listener_direction.getRotationTo(Ogre::Vector3::NEGATIVE_UNIT_Z);
    }

    early_reflections_pan = horizontal_rotation * early_reflections_pan;
    early_reflections_pan.normalise();

    early_reflections_pan = magnitude * early_reflections_pan;

    return std::make_tuple(early_reflections_pan, early_reflections_gain, early_reflections_delay);
}

ALuint SoundManager::CreateAlEffect(const EFXEAXREVERBPROPERTIES* efx_properties) const
{
    ALuint effect = 0;
    ALenum error;

    this->alGenEffects(1, &effect);

    switch (m_efx_reverb_engine)
    {
        case EfxReverbEngine::EAXREVERB:
            this->alEffecti( effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

            this->alEffectf( effect, AL_EAXREVERB_DENSITY,               efx_properties->flDensity);
            this->alEffectf( effect, AL_EAXREVERB_DIFFUSION,             efx_properties->flDiffusion);
            this->alEffectf( effect, AL_EAXREVERB_GAIN,                  efx_properties->flGain);
            this->alEffectf( effect, AL_EAXREVERB_GAINHF,                efx_properties->flGainHF);
            this->alEffectf( effect, AL_EAXREVERB_GAINLF,                efx_properties->flGainLF);
            this->alEffectf( effect, AL_EAXREVERB_DECAY_TIME,            efx_properties->flDecayTime);
            this->alEffectf( effect, AL_EAXREVERB_DECAY_HFRATIO,         efx_properties->flDecayHFRatio);
            this->alEffectf( effect, AL_EAXREVERB_DECAY_LFRATIO,         efx_properties->flDecayLFRatio);
            this->alEffectf( effect, AL_EAXREVERB_REFLECTIONS_GAIN,      efx_properties->flReflectionsGain);
            this->alEffectf( effect, AL_EAXREVERB_REFLECTIONS_DELAY,     efx_properties->flReflectionsDelay);
            this->alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN,       efx_properties->flReflectionsPan);
            this->alEffectf( effect, AL_EAXREVERB_LATE_REVERB_GAIN,      efx_properties->flLateReverbGain);
            this->alEffectf( effect, AL_EAXREVERB_LATE_REVERB_DELAY,     efx_properties->flLateReverbDelay);
            this->alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN,       efx_properties->flLateReverbPan);
            this->alEffectf( effect, AL_EAXREVERB_ECHO_TIME,             efx_properties->flEchoTime);
            this->alEffectf( effect, AL_EAXREVERB_ECHO_DEPTH,            efx_properties->flEchoDepth);
            this->alEffectf( effect, AL_EAXREVERB_MODULATION_TIME,       efx_properties->flModulationTime);
            this->alEffectf( effect, AL_EAXREVERB_MODULATION_DEPTH,      efx_properties->flModulationDepth);
            this->alEffectf( effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, efx_properties->flAirAbsorptionGainHF);
            this->alEffectf( effect, AL_EAXREVERB_HFREFERENCE,           efx_properties->flHFReference);
            this->alEffectf( effect, AL_EAXREVERB_LFREFERENCE,           efx_properties->flLFReference);
            this->alEffectf( effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR,   efx_properties->flRoomRolloffFactor);
            this->alEffecti( effect, AL_EAXREVERB_DECAY_HFLIMIT,         efx_properties->iDecayHFLimit);

            break;
        case EfxReverbEngine::REVERB:
            this->alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

            this->alEffectf(effect, AL_REVERB_DENSITY,                efx_properties->flDensity);
            this->alEffectf(effect, AL_REVERB_DIFFUSION,              efx_properties->flDiffusion);
            this->alEffectf(effect, AL_REVERB_GAIN,                   efx_properties->flGain);
            this->alEffectf(effect, AL_REVERB_GAINHF,                 efx_properties->flGainHF);
            this->alEffectf(effect, AL_REVERB_DECAY_TIME,             efx_properties->flDecayTime);
            this->alEffectf(effect, AL_REVERB_DECAY_HFRATIO,          efx_properties->flDecayHFRatio);
            this->alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN,       efx_properties->flReflectionsGain);
            this->alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY,      efx_properties->flReflectionsDelay);
            this->alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN,       efx_properties->flLateReverbGain);
            this->alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY,      efx_properties->flLateReverbDelay);
            this->alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF,  efx_properties->flAirAbsorptionGainHF);
            this->alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR,    efx_properties->flRoomRolloffFactor);
            this->alEffecti(effect, AL_REVERB_DECAY_HFLIMIT,          efx_properties->iDecayHFLimit);

            break;
        case EfxReverbEngine::NONE:
        default:
            LOG("SoundManager: No usable reverb engine set, not creating reverb effect");
    }

    error = alGetError();
    if(error != AL_NO_ERROR)
    {
        LOG("SoundManager: Could not create EFX effect:" + TOSTRING(alGetString(error)));

        if(this->alIsEffect(effect))
            this->alDeleteEffects(1, &effect);
        return 0;
    }

    return effect;
}

void SoundManager::DeleteAlEffect(const ALuint efx_effect_id) const
{
    ALenum error;
    alGetError();

    this->alDeleteEffects(1, &efx_effect_id);

    error = alGetError();
    if(error != AL_NO_ERROR)
    {
        LOG("SoundManager: Could not delete EFX effect: " + TOSTRING(alGetString(error)));
    }
}

bool compareByAudibility(std::pair<int, float> a, std::pair<int, float> b)
{
    return a.second > b.second;
}

// called when the camera moves
void SoundManager::recomputeAllSources()
{
    // Creates this issue: https://github.com/RigsOfRods/rigs-of-rods/issues/1054
#if 0
	if (!audio_device) return;

	for (int i=0; i < m_audio_sources_in_use_count; i++)
	{
		audio_sources[i]->computeAudibility(m_listener_position);
		audio_sources_most_audible[i].first = i;
		audio_sources_most_audible[i].second = audio_sources[i]->audibility;
	}
	// sort first 'num_hardware_sources' sources by audibility
	// see: https://en.wikipedia.org/wiki/Selection_algorithm
	if ((m_audio_sources_in_use_count - 1) > hardware_sources_num)
	{
		std::nth_element(audio_sources_most_audible, audio_sources_most_audible+hardware_sources_num, audio_sources_most_audible + m_audio_sources_in_use_count - 1, compareByAudibility);
	}
	// retire out of range sources first
	for (int i=0; i < m_audio_sources_in_use_count; i++)
	{
		if (audio_sources[audio_sources_most_audible[i].first]->hardware_index != -1 && (i >= hardware_sources_num || audio_sources_most_audible[i].second == 0))
			retire(audio_sources_most_audible[i].first);
	}
	// assign new sources
	for (int i=0; i < std::min(m_audio_sources_in_use_count, hardware_sources_num); i++)
	{
		if (audio_sources[audio_sources_most_audible[i].first]->hardware_index == -1 && audio_sources_most_audible[i].second > 0)
		{
			for (int j=0; j < hardware_sources_num; j++)
			{
				if (hardware_sources_map[j] == -1)
				{
					assign(audio_sources_most_audible[i].first, j);
					break;
				}
			}
		}
	}
#endif
}

void SoundManager::UpdateObstructionFilter(const int hardware_index) const
{
    if(hardware_sources_map[hardware_index] == -1) { return; } // no sound assigned to hardware source

    if(!App::audio_enable_obstruction->getBool())
    {
        // detach the obstruction filter in case it was attached when the feature was previously enabled
        alSourcei(hardware_sources[hardware_index], AL_DIRECT_FILTER, AL_FILTER_NULL);
        return;
    }

    bool obstruction_detected = false;
    const SoundPtr& corresponding_sound = audio_sources[hardware_sources_map[hardware_index]];

    // TODO: Simulate diffraction path.

    // always obstruct sounds if the player is in a vehicle
    if(App::GetSoundScriptManager()->ListenerIsInsideThePlayerCoupledActor())
    {
        obstruction_detected = true;
    }
    else
    {
        /*
        * Perform various line of sight checks until either a collision was detected
        * and the filter has to be applied or no obstruction was detected.
        */

        std::pair<bool, Ogre::Real> intersection;
        // no normalisation due to how the intersectsTris function determines its number of steps
        Ogre::Vector3 direction_to_sound = corresponding_sound->getPosition() - m_listener_position;
        Ogre::Real distance_to_sound = direction_to_sound.length();
        Ray direct_path_to_sound = Ray(m_listener_position, direction_to_sound);

        // perform line of sight check against terrain
        intersection = App::GetGameContext()->GetTerrain()->GetCollisions()->intersectsTerrain(direct_path_to_sound, distance_to_sound);
        obstruction_detected = intersection.first;

        if(!obstruction_detected)
        {
            // perform line of sight check against collision meshes
            // for this to work correctly, the direction vector of the ray must have
            // the length of the distance from the listener to the sound
            intersection = App::GetGameContext()->GetTerrain()->GetCollisions()->intersectsTris(direct_path_to_sound);
            obstruction_detected = intersection.first;
        }

        // do not normalise before intersectsTris() due to how that function works
        direction_to_sound.normalise();
        direct_path_to_sound.setDirection(direction_to_sound);

        if(!obstruction_detected)
        {
            // perform line of sight check agains collision boxes
            for (const collision_box_t& collision_box : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionBoxes())
            {
                if (!collision_box.enabled || collision_box.virt) { continue; }

                intersection = direct_path_to_sound.intersects(Ogre::AxisAlignedBox(collision_box.lo, collision_box.hi));
                if (intersection.first && intersection.second <= distance_to_sound)
                {
                    obstruction_detected = true;
                    break;
                }
            }
        }

        if(!obstruction_detected)
        {
            // perform line of sight check against actors
            const ActorPtrVec& actors = App::GetGameContext()->GetActorManager()->GetActors();
            bool soundsource_belongs_to_current_actor = false;
            for(const ActorPtr actor : actors)
            {
                // Trucks shouldn't obstruct their own sound sources since the
                // obstruction is most likely already contained in the recording.
                for (int soundsource_index = 0; soundsource_index < actor->ar_num_soundsources; ++soundsource_index)
                {
                    const soundsource_t& soundsource = actor->ar_soundsources[soundsource_index];
                    const int num_sounds = soundsource.ssi->getTemplate()->getNumSounds();
                    for (int num_sound = 0; num_sound < num_sounds; ++num_sound)
                    {
                        if (soundsource.ssi->getSound(num_sound) == corresponding_sound)
                        {
                            soundsource_belongs_to_current_actor = true;
                        }
                    }
                    if (soundsource_belongs_to_current_actor) { break; }
                }

                if (soundsource_belongs_to_current_actor)
                {
                    continue;
                }

                intersection = direct_path_to_sound.intersects(actor->ar_bounding_box);
                obstruction_detected = intersection.first;
                if (obstruction_detected)
                {
                    break;
                }
            }
        }
    }

    if(obstruction_detected)
    {
        // Apply obstruction filter to the source
        alSourcei(hardware_sources[hardware_index], AL_DIRECT_FILTER, m_efx_outdoor_obstruction_lowpass_filter_id);
    }
    else
    {
        // reset direct filter for the source in case it has been set previously
        alSourcei(hardware_sources[hardware_index], AL_DIRECT_FILTER, AL_FILTER_NULL);
    }
}

void SoundManager::recomputeSource(int source_index, int reason, float vfl, Vector3* vvec)
{
    if (!audio_device)
        return;
    audio_sources[source_index]->computeAudibility(m_listener_position);

    if (audio_sources[source_index]->audibility == 0.0f)
    {
        if (audio_sources[source_index]->hardware_index != -1)
        {
            // retire the source if it is currently assigned
            retire(source_index);
        }
    }
    else
    {
        // this is a potentially audible m_audio_sources[source_index]
        if (audio_sources[source_index]->hardware_index != -1)
        {
            ALuint hw_source = hardware_sources[audio_sources[source_index]->hardware_index];
            // m_audio_sources[source_index] already playing
            // update the AL settings
            switch (reason)
            {
            case Sound::REASON_PLAY:
                this->UpdateObstructionFilter(audio_sources[source_index]->hardware_index);
                alSourcePlay(hw_source);
                break;
            case Sound::REASON_STOP: alSourceStop(hw_source);
                break;
            case Sound::REASON_GAIN: alSourcef(hw_source, AL_GAIN, vfl * App::audio_master_volume->getFloat());
                break;
            case Sound::REASON_LOOP: alSourcei(hw_source, AL_LOOPING, (vfl > 0.5) ? AL_TRUE : AL_FALSE);
                break;
            case Sound::REASON_PTCH: alSourcef(hw_source, AL_PITCH, vfl);
                break;
            case Sound::REASON_POSN: alSource3f(hw_source, AL_POSITION, vvec->x, vvec->y, vvec->z);
                break;
            case Sound::REASON_VLCT: alSource3f(hw_source, AL_VELOCITY, vvec->x, vvec->y, vvec->z);
                break;
            default: break;
            }
        }
        else
        {
            // try to make it play by the hardware
            // check if there is one free m_audio_sources[source_index] in the pool
            if (hardware_sources_in_use_count < hardware_sources_num)
            {
                for (int i = 0; i < hardware_sources_num; i++)
                {
                    if (hardware_sources_map[i] == -1)
                    {
                        assign(source_index, i);
                        break;
                    }
                }
            }
            else
            {
                // now, compute who is the faintest
                // note: we know the table m_hardware_sources_map is full!
                float fv = 1.0f;
                int al_faintest = 0;
                for (int i = 0; i < hardware_sources_num; i++)
                {
                    if (hardware_sources_map[i] >= 0 && audio_sources[hardware_sources_map[i]]->audibility < fv)
                    {
                        fv = audio_sources[hardware_sources_map[i]]->audibility;
                        al_faintest = i;
                    }
                }
                // check to ensure that the sound is louder than the faintest sound currently playing
                if (fv < audio_sources[source_index]->audibility)
                {
                    // this new m_audio_sources[source_index] is louder than the faintest!
                    retire(hardware_sources_map[al_faintest]);
                    assign(source_index, al_faintest);
                }
                // else this m_audio_sources[source_index] is too faint, we don't play it!
            }
        }
    }
}

void SoundManager::assign(int source_index, int hardware_index)
{
    if (!audio_device)
        return;
    audio_sources[source_index]->hardware_index = hardware_index;
    hardware_sources_map[hardware_index] = source_index;

    ALuint hw_source = hardware_sources[hardware_index];
    SoundPtr& audio_source = audio_sources[source_index];

    // the hardware source is supposed to be stopped!
    alSourcei(hw_source, AL_BUFFER, audio_source->buffer);
    alSourcef(hw_source, AL_GAIN, audio_source->gain * App::audio_master_volume->getFloat());
    alSourcei(hw_source, AL_LOOPING, (audio_source->loop) ? AL_TRUE : AL_FALSE);
    alSourcef(hw_source, AL_PITCH, audio_source->pitch);
    alSource3f(hw_source, AL_POSITION, audio_source->position.x, audio_source->position.y, audio_source->position.z);
    alSource3f(hw_source, AL_VELOCITY, audio_source->velocity.x, audio_source->velocity.y, audio_source->velocity.z);

    if (audio_source->should_play)
    {
        this->UpdateObstructionFilter(hardware_index);
        alSourcePlay(hw_source);
    }

    hardware_sources_in_use_count++;
}

void SoundManager::retire(int source_index)
{
    if (!audio_device)
        return;
    if (audio_sources[source_index]->hardware_index == -1)
        return;
    alSourceStop(hardware_sources[audio_sources[source_index]->hardware_index]);
    hardware_sources_map[audio_sources[source_index]->hardware_index] = -1;
    audio_sources[source_index]->hardware_index = -1;
    hardware_sources_in_use_count--;
}

void SoundManager::pauseAllSounds()
{
    if (!audio_device)
        return;
    // no mutex needed
    alListenerf(AL_GAIN, 0.0f);
}

void SoundManager::resumeAllSounds()
{
    if (!audio_device)
        return;
    // no mutex needed
    alListenerf(AL_GAIN, App::audio_master_volume->getFloat());
}

void SoundManager::setMasterVolume(float v)
{
    if (!audio_device)
        return;
    // no mutex needed
    App::audio_master_volume->setVal(v); // TODO: Use 'pending' mechanism and set externally, only 'apply' here.
    alListenerf(AL_GAIN, v);
}

SoundPtr SoundManager::createSound(String filename, Ogre::String resource_group_name /* = "" */)
{
    if (!audio_device)
        return NULL;

    if (audio_buffers_in_use_count >= MAX_AUDIO_BUFFERS)
    {
        LOG("SoundManager: Reached MAX_AUDIO_BUFFERS limit (" + TOSTRING(MAX_AUDIO_BUFFERS) + ")");
        return NULL;
    }

    ALuint buffer = 0;

    // is the file already loaded?
    for (int i = 0; i < audio_buffers_in_use_count; i++)
    {
        if (filename == audio_buffer_file_name[i])
        {
            buffer = audio_buffers[i];
            break;
        }
    }

    if (!buffer)
    {
        // load the file
        alGenBuffers(1, &audio_buffers[audio_buffers_in_use_count]);
        if (loadWAVFile(filename, audio_buffers[audio_buffers_in_use_count], resource_group_name))
        {
            // there was an error!
            alDeleteBuffers(1, &audio_buffers[audio_buffers_in_use_count]);
            audio_buffer_file_name[audio_buffers_in_use_count] = "";
            return NULL;
        }
        buffer = audio_buffers[audio_buffers_in_use_count];
        audio_buffer_file_name[audio_buffers_in_use_count] = filename;
        audio_buffers_in_use_count++;
    }

    audio_sources[m_audio_sources_in_use_count] = new Sound(buffer, this, m_audio_sources_in_use_count);

    return audio_sources[m_audio_sources_in_use_count++];
}

bool SoundManager::loadWAVFile(String filename, ALuint buffer, Ogre::String resource_group_name /*= ""*/)
{
    if (!audio_device)
        return true;
    LOG("Loading WAV file "+filename);

    // create the Stream
    ResourceGroupManager* rgm = ResourceGroupManager::getSingletonPtr();
    if (resource_group_name == "")
    {
        resource_group_name = rgm->findGroupContainingResource(filename);
    }
    DataStreamPtr stream = rgm->openResource(filename, resource_group_name);

    // load RIFF/WAVE
    char magic[5];
    magic[4] = 0;
    unsigned int lbuf; // uint32_t
    unsigned short sbuf; // uint16_t

    // check magic
    if (stream->read(magic, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    if (String(magic) != String("RIFF"))
    {
        LOG("Invalid WAV file (no RIFF): "+filename);
        return true;
    }
    // skip 4 bytes (magic)
    stream->skip(4);
    // check file format
    if (stream->read(magic, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    if (String(magic) != String("WAVE"))
    {
        LOG("Invalid WAV file (no WAVE): "+filename);
        return true;
    }
    // check 'fmt ' sub chunk (1)
    if (stream->read(magic, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    if (String(magic) != String("fmt "))
    {
        LOG("Invalid WAV file (no fmt): "+filename);
        return true;
    }
    // read (1)'s size
    if (stream->read(&lbuf, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    unsigned long subChunk1Size = lbuf;
    if (subChunk1Size < 16)
    {
        LOG("Invalid WAV file (invalid subChunk1Size): "+filename);
        return true;
    }
    // check PCM audio format
    if (stream->read(&sbuf, 2) != 2)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    unsigned short audioFormat = sbuf;
    if (audioFormat != 1)
    {
        LOG("Invalid WAV file (invalid audioformat "+TOSTRING(audioFormat)+"): "+filename);
        return true;
    }
    // read number of channels
    if (stream->read(&sbuf, 2) != 2)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    unsigned short channels = sbuf;
    // read frequency (sample rate)
    if (stream->read(&lbuf, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    unsigned long freq = lbuf;
    // skip 6 bytes (Byte rate (4), Block align (2))
    stream->skip(6);
    // read bits per sample
    if (stream->read(&sbuf, 2) != 2)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    unsigned short bps = sbuf;
    // check 'data' sub chunk (2)
    if (stream->read(magic, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }
    if (String(magic) != String("data") && String(magic) != String("fact"))
    {
        LOG("Invalid WAV file (no data/fact): "+filename);
        return true;
    }
    // fact is an option section we don't need to worry about
    if (String(magic) == String("fact"))
    {
        stream->skip(8);
        // now we should hit the data chunk
        if (stream->read(magic, 4) != 4)
        {
            LOG("Could not read file "+filename);
            return true;
        }
        if (String(magic) != String("data"))
        {
            LOG("Invalid WAV file (no data): "+filename);
            return true;
        }
    }
    // the next four bytes are the remaining size of the file
    if (stream->read(&lbuf, 4) != 4)
    {
        LOG("Could not read file "+filename);
        return true;
    }

    unsigned long dataSize = lbuf;
    int format = 0;

    if (channels == 1 && bps == 8)
        format = AL_FORMAT_MONO8;
    else if (channels == 1 && bps == 16)
        format = AL_FORMAT_MONO16;
    else if (channels == 2 && bps == 8)
        format = AL_FORMAT_STEREO16;
    else if (channels == 2 && bps == 16)
        format = AL_FORMAT_STEREO16;
    else
    {
        LOG("Invalid WAV file (wrong channels/bps): "+filename);
        return true;
    }

    if (channels != 1) LOG("Invalid WAV file: the file needs to be mono, and nothing else. Will try to continue anyways ...");

    // ok, creating buffer
    void* bdata = malloc(dataSize);
    if (!bdata)
    {
        LOG("Memory error reading file "+filename);
        return true;
    }
    if (stream->read(bdata, dataSize) != dataSize)
    {
        LOG("Could not read file "+filename);
        free(bdata);
        return true;
    }

    //LOG("alBufferData: format "+TOSTRING(format)+" size "+TOSTRING(dataSize)+" freq "+TOSTRING(freq));
    alGetError(); // Reset errors
    ALint error;
    alBufferData(buffer, format, bdata, dataSize, freq);
    error = alGetError();

    free(bdata);
    // stream will be closed by itself

    if (error != AL_NO_ERROR)
    {
        LOG("OpenAL error while loading buffer for "+filename+" : "+TOSTRING(error));
        return true;
    }

    return false;
}

#endif // USE_OPENAL
