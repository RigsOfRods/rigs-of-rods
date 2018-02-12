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

#include "RoRPrerequisites.h"

struct RoadType_t
{
    char name[256];
    Ogre::SceneNode* node;
};

class Road : public ZeroedMemoryAllocator
{
    friend class SimController;

public:

    Road(Ogre::Vector3 start);

private:

    void addRoadType(const char* name);
    void append();
    void dpitch(float v);
    void dturn(float v);
    void preparePending();
    void reset(Ogre::Vector3 start);
    void toggleType();
    void updatePending();

    static const unsigned int MAX_RTYPES = 10;
    RoadType_t rtypes[MAX_RTYPES];

    Ogre::SceneNode* tenode;

    Ogre::Vector3 ppos;
    Ogre::Vector3 protl;
    Ogre::Vector3 protr;
    Ogre::Vector3 rpos;
    Ogre::Vector3 rrot;

    float lastpturn;
    float ppitch;
    float pturn;

    int cur_rtype;
    int free_rtype;

    char curtype[256];
};
