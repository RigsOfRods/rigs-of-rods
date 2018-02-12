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

#include <OgrePrerequisites.h>

#include <MyGUI.h>

#include "RoRPrerequisites.h"
#include "Beam.h"

struct node_simple_t
{
    Ogre::Vector3 position;
    Ogre::Vector3 velocity;
    Ogre::Vector3 forces;
};

struct beam_simple_t
{
    bool broken;
    bool disabled;
};

class Replay : public ZeroedMemoryAllocator
{
public:
    Replay(Actor* b, int nframes);
    ~Replay();

    void* getWriteBuffer(int type);
    void* getReadBuffer(int offset, int type, unsigned long& time);
    unsigned long getLastReadTime();
    void writeDone();

    void setHidden(bool value);

    void setVisible(bool value);
    bool getVisible();

    bool isValid() { return !outOfMemory; };
protected:
    Ogre::Timer* replayTimer;
    int numNodes;
    int numBeams;
    int numFrames;
    bool outOfMemory;

    bool hidden;
    bool visible;

    int writeIndex;
    int firstRun;
    unsigned long curFrameTime;
    int curOffset;

    // malloc'ed
    node_simple_t* nodes;
    beam_simple_t* beams;
    unsigned long* times;

    // windowing
    MyGUI::WidgetPtr panel;
    MyGUI::StaticTextPtr txt;
    MyGUI::ProgressPtr pr;

    void updateGUI();
};
