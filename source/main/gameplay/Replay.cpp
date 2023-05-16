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

#include "Replay.h"

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "Utils.h"

using namespace Ogre;
using namespace RoR;

Replay::Replay(ActorPtr actor, int _numFrames)
{
    m_actor = actor;
    numFrames = _numFrames;

    curFrameTime = 0;

    replayTimer = new Timer();

    // DO NOT get memory here, get memory when we use it first time!
    nodes = 0;
    beams = 0;
    times = 0;

    outOfMemory = false;

    const int numNodes = static_cast<int>(actor->ar_nodes.size());
    const int numBeams = actor->ar_num_beams;
    unsigned long bsize = (numNodes * numFrames * sizeof(node_simple_t) + numBeams * numFrames * sizeof(beam_simple_t) + numFrames * sizeof(unsigned long)) / 1024.0f;
    LOG("replay buffer size: " + TOSTRING(bsize) + " kB");

    writeIndex = 0;
    firstRun = 1;

    int steps = App::sim_replay_stepping->getInt();

    if (steps <= 0)
        this->ar_replay_precision = 0.0f;
    else
        this->ar_replay_precision = 1.0f / ((float)steps);

    // windowing
    int width = 300;
    int height = 60;
    int x = (MyGUI::RenderManager::getInstance().getViewSize().width - width) / 2;
    int y = 0;
}

Replay::~Replay()
{
    if (nodes)
    {
        free(nodes);
        nodes = 0;
        free(beams);
        beams = 0;
        free(times);
        times = 0;
    }
    delete replayTimer;
}

void* Replay::getWriteBuffer(int type)
{
    if (outOfMemory)
        return 0;
    if (!nodes)
    {
        // get memory
        nodes = (node_simple_t*)calloc(static_cast<int>(m_actor->ar_nodes.size()) * numFrames, sizeof(node_simple_t));
        if (!nodes)
        {
            outOfMemory = true;
            return 0;
        }
        beams = (beam_simple_t*)calloc(m_actor->ar_num_beams * numFrames, sizeof(beam_simple_t));
        if (!beams)
        {
            free(nodes);
            nodes = 0;
            outOfMemory = true;
            return 0;
        }
        times = (unsigned long*)calloc(numFrames, sizeof(unsigned long));
        if (!times)
        {
            free(nodes);
            nodes = 0;
            free(beams);
            beams = 0;
            outOfMemory = true;
            return 0;
        }
    }
    void* ptr = 0;
    times[writeIndex] = replayTimer->getMicroseconds();
    if (type == 0)
    {
        // nodes
        ptr = (void *)(nodes + (writeIndex * static_cast<int>(m_actor->ar_nodes.size())));
    }
    else if (type == 1)
    {
        // beams
        ptr = (void *)(beams + (writeIndex * m_actor->ar_num_beams));
    }
    return ptr;
}

void Replay::writeDone()
{
    if (outOfMemory)
        return;
    writeIndex++;
    if (writeIndex == numFrames)
    {
        firstRun = 0;
        writeIndex = 0;
    }
}

//we take negative offsets only
void* Replay::getReadBuffer(int offset, int type, unsigned long& time)
{
    if (offset >= 0)
        offset = -1;
    if (offset <= -numFrames)
        offset = -numFrames + 1;

    int delta = writeIndex + offset;
    if (delta < 0)
        if (firstRun && type == 0)
            return (void *)(nodes);
        else if (firstRun && type == 1)
            return (void *)(beams);
        else
            delta += numFrames;

    // set the time
    time = times[delta];
    curFrameTime = time;

    if (outOfMemory)
        return 0;

    // return buffer pointer
    if (type == 0)
        return (void *)(nodes + delta * static_cast<int>(m_actor->ar_nodes.size()));
    else if (type == 1)
        return (void *)(beams + delta * m_actor->ar_num_beams);
    return 0;
}

unsigned long Replay::getLastReadTime()
{
    return curFrameTime;
}
 
void Replay::onPhysicsStep()
{
    m_replay_timer += PHYSICS_DT;
    if (m_replay_timer >= ar_replay_precision)
    {
        // store nodes
        node_simple_t* nbuff = (node_simple_t *)this->getWriteBuffer(0);
        if (nbuff)
        {
            for (int i = 0; i < static_cast<int>(m_actor->ar_nodes.size()); i++)
            {
                nbuff[i].position = m_actor->ar_nodes[i].AbsPosition;
                nbuff[i].velocity = m_actor->ar_nodes[i].Velocity;
            }
        }

        // store beams
        beam_simple_t* bbuff = (beam_simple_t *)this->getWriteBuffer(1);
        if (bbuff)
        {
            for (int i = 0; i < m_actor->ar_num_beams; i++)
            {
                bbuff[i].broken = m_actor->ar_beams[i].bm_broken;
                bbuff[i].disabled = m_actor->ar_beams[i].bm_disabled;
            }
        }

        this->writeDone();
        m_replay_timer = 0.0f;
    }
}

void Replay::replayStepActor()
{
    if (ar_replay_pos != m_replay_pos_prev)
    {
        unsigned long time = 0;

        node_simple_t* nbuff = (node_simple_t *)this->getReadBuffer(ar_replay_pos, 0, time);
        if (nbuff)
        {
            for (int i = 0; i < static_cast<int>(m_actor->ar_nodes.size()); i++)
            {
                m_actor->ar_nodes[i].AbsPosition = nbuff[i].position;
                m_actor->ar_nodes[i].RelPosition = nbuff[i].position - m_actor->ar_origin;

                m_actor->ar_nodes[i].Velocity = nbuff[i].velocity;
                m_actor->ar_nodes[i].Forces = Vector3::ZERO;
            }

            m_actor->updateSlideNodePositions();
            m_actor->UpdateBoundingBoxes();
            m_actor->calculateAveragePosition();
        }

        beam_simple_t* bbuff = (beam_simple_t *)this->getReadBuffer(ar_replay_pos, 1, time);
        if (bbuff)
        {
            for (int i = 0; i < m_actor->ar_num_beams; i++)
            {
                m_actor->ar_beams[i].bm_broken = bbuff[i].broken;
                m_actor->ar_beams[i].bm_disabled = bbuff[i].disabled;
            }
        }
        m_replay_pos_prev = ar_replay_pos;
    }
}

void Replay::UpdateInputEvents()
{
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
    {
        if (m_actor->ar_state == ActorState::LOCAL_REPLAY)
            m_actor->ar_state = ActorState::LOCAL_SIMULATED;
        else
            m_actor->ar_state = ActorState::LOCAL_REPLAY;
    }

    if (m_actor->ar_state == ActorState::LOCAL_REPLAY)
    {
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && this->ar_replay_pos <= 0)
        {
            this->ar_replay_pos++;
        }
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && this->ar_replay_pos > -this->getNumFrames())
        {
            this->ar_replay_pos--;
        }
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && this->ar_replay_pos + 10 <= 0)
        {
            this->ar_replay_pos += 10;
        }
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && this->ar_replay_pos - 10 > -this->getNumFrames())
        {
            this->ar_replay_pos -= 10;
        }

        if (App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
        {
            if (this->ar_replay_pos <= 0 && this->ar_replay_pos >= -this->getNumFrames())
            {
                if (App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
                {
                    this->ar_replay_pos += App::GetInputEngine()->getMouseState().X.rel * 1.5f;
                }
                else
                {
                    this->ar_replay_pos += App::GetInputEngine()->getMouseState().X.rel * 0.05f;
                }
                if (this->ar_replay_pos > 0)
                {
                    this->ar_replay_pos = 0;
                }
                if (this->ar_replay_pos < -this->getNumFrames())
                {
                    this->ar_replay_pos = -this->getNumFrames();
                }
            }
        }
    }
}
