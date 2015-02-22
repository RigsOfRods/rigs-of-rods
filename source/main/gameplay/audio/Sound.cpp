/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef USE_OPENAL

#include "Sound.h"
#include "SoundManager.h"

using namespace Ogre;

Sound::Sound(ALuint buffer, SoundManager *soundManager, int sourceIndex) :
	  buffer(buffer)
	, sound_manager(soundManager)
	, source_index(sourceIndex)
	, audibility(0.0f)
	, gain(0.0f)
	, pitch(1.0f)
	, position(Vector3::ZERO)
	, velocity(Vector3::ZERO)
	, enabled(true)
	, loop(false)
	, should_play(false)
	, hardware_index(-1)
{
}

void Sound::computeAudibility(Vector3 pos)
{
	// disable sound?
	if (!enabled)
	{
		audibility = 0.0f;
		return;
	}

	// first check if the sound is finished!
	if (!loop && should_play && hardware_index != -1)
	{
		int value = 0;
		alGetSourcei((ALuint)sound_manager->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		if (value != AL_PLAYING)
		{
			should_play = false;
		}
	}
	
	// should it play at all?
	if (!should_play || gain == 0.0f)
	{
		audibility = 0.0f;
		return;
	}

	float distance = (pos - position).length();
	
	if (distance > sound_manager->MAX_DISTANCE)
	{
		audibility = 0.0f;
	} else if (distance < sound_manager->REFERENCE_DISTANCE)
	{
		audibility = gain;
	} else
	{
		audibility = gain * (sound_manager->REFERENCE_DISTANCE / (sound_manager->REFERENCE_DISTANCE + (sound_manager->ROLLOFF_FACTOR * (distance - sound_manager->REFERENCE_DISTANCE))));
	}
}

bool Sound::isPlaying()
{
	if (hardware_index != -1)
	{
		int value = 0;
		alGetSourcei((ALuint)sound_manager->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		return (value == AL_PLAYING);
	}
	return false;
}

void Sound::setEnabled(bool e)
{
	enabled = e;
	sound_manager->recomputeSource(source_index, REASON_PLAY, 0.0f, NULL);
}

bool Sound::getEnabled()
{
	return enabled;
}

void Sound::play()
{
	should_play = true;
	sound_manager->recomputeSource(source_index, REASON_PLAY, 0.0f, NULL);
}

void Sound::stop()
{
	should_play = false;
	sound_manager->recomputeSource(source_index, REASON_STOP, 0.0f, NULL);
}

void Sound::setGain(float gain)
{
	this->gain = gain;
	sound_manager->recomputeSource(source_index, REASON_GAIN, gain, NULL);
}

void Sound::setLoop(bool loop)
{
	this->loop = loop;
	sound_manager->recomputeSource(source_index, REASON_LOOP, (loop) ? 1.0f : 0.0f, NULL);
}

void Sound::setPitch(float pitch)
{
	this->pitch = pitch;
	sound_manager->recomputeSource(source_index, REASON_PTCH, pitch, NULL);
}

void Sound::setPosition(Ogre::Vector3 pos)
{
	this->position = pos;
	sound_manager->recomputeSource(source_index, REASON_POSN, 0.0f, &pos);
}

void Sound::setVelocity(Ogre::Vector3 vel)
{
	this->velocity = vel;
	sound_manager->recomputeSource(source_index, REASON_VLCT, 0.0f, &vel);
}

#endif // USE_OPENAL
