/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#pragma once

#include "Application.h"

namespace RoR {

struct node_simple_t
{
    Ogre::Vector3 position;
    Ogre::Vector3 velocity;
};

struct beam_simple_t
{
    bool broken:1;
    bool disabled:1;
};

class Replay : public ZeroedMemoryAllocator
{
public:
    Replay(ActorPtr b, int nframes);
    ~Replay();

    void*               getWriteBuffer(int type);
    void*               getReadBuffer(int offset, int type, unsigned long& time);
    unsigned long       getLastReadTime();
    void                writeDone();
    void                onPhysicsStep();
    void                replayStepActor();
    float               getPrecision() const { return ar_replay_precision; }
    float               getReplayPositionSec() const { return ((float)curFrameTime) / 1000000.0f; }
    int                 getNumFrames() const { return numFrames; }
    int                 getCurrentFrame() const { return ar_replay_pos; }
    bool                isValid() { return numFrames && !outOfMemory; };
    void                UpdateInputEvents();

protected:
    ActorPtr              m_actor = nullptr;
    float               m_replay_timer = 0.f;
    float               ar_replay_precision = 1.f;
    int                 ar_replay_pos = 0;
    int                 m_replay_pos_prev = 0;
    Ogre::Timer*        replayTimer;
    int                 numFrames;
    bool                outOfMemory;
    int                 writeIndex;
    int                 firstRun;
    unsigned long       curFrameTime;

    // malloc'ed
    node_simple_t*      nodes;
    beam_simple_t*      beams;
    unsigned long*      times;
};

} // namespace RoR
