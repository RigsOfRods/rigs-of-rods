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
    }

    alDopplerFactor(1.0f);
    alDopplerVelocity(343.0f);

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

void SoundManager::setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity)
{
    if (!audio_device)
        return;
    camera_position = position;
    recomputeAllSources();

    float orientation[6];
    // direction
    orientation[0] = direction.x;
    orientation[1] = direction.y;
    orientation[2] = direction.z;
    // up
    orientation[3] = up.x;
    orientation[4] = up.y;
    orientation[5] = up.z;

    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    alListenerfv(AL_ORIENTATION, orientation);
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

	for (int i=0; i < audio_buffers_in_use_count; i++)
	{
		audio_sources[i]->computeAudibility(camera_position);
		audio_sources_most_audible[i].first = i;
		audio_sources_most_audible[i].second = audio_sources[i]->audibility;
	}
    // sort first 'num_hardware_sources' sources by audibility
    // see: https://en.wikipedia.org/wiki/Selection_algorithm
	if ((audio_buffers_in_use_count - 1) > hardware_sources_num)
	{
		std::nth_element(audio_sources_most_audible, audio_sources_most_audible+hardware_sources_num, audio_sources_most_audible + audio_buffers_in_use_count - 1, compareByAudibility);
	}
    // retire out of range sources first
	for (int i=0; i < audio_buffers_in_use_count; i++)
	{
		if (audio_sources[audio_sources_most_audible[i].first]->hardware_index != -1 && (i >= hardware_sources_num || audio_sources_most_audible[i].second == 0))
			retire(audio_sources_most_audible[i].first);
	}
    // assign new sources
	for (int i=0; i < std::min(audio_buffers_in_use_count, hardware_sources_num); i++)
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

void SoundManager::recomputeSource(int source_index, int reason, float vfl, Vector3* vvec)
{
    if (!audio_device)
        return;
    audio_sources[source_index]->computeAudibility(camera_position);

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
            case Sound::REASON_PLAY: alSourcePlay(hw_source);
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
    SoundPtr audio_source = audio_sources[source_index];

    // the hardware source is supposed to be stopped!
    alSourcei(hw_source, AL_BUFFER, audio_source->buffer);
    alSourcef(hw_source, AL_GAIN, audio_source->gain * App::audio_master_volume->getFloat());
    alSourcei(hw_source, AL_LOOPING, (audio_source->loop) ? AL_TRUE : AL_FALSE);
    alSourcef(hw_source, AL_PITCH, audio_source->pitch);
    alSource3f(hw_source, AL_POSITION, audio_source->position.x, audio_source->position.y, audio_source->position.z);
    alSource3f(hw_source, AL_VELOCITY, audio_source->velocity.x, audio_source->velocity.y, audio_source->velocity.z);

    if (audio_source->should_play)
    {
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
    }

    audio_sources[audio_buffers_in_use_count] = new Sound(buffer, this, audio_buffers_in_use_count);

    return audio_sources[audio_buffers_in_use_count++];
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
