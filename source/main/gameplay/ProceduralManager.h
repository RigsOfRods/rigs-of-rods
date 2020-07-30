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

// thomas, 17th of June 2008
#pragma once

#include "Application.h"
#include "Road2.h"

namespace RoR {

class ProceduralPoint : public ZeroedMemoryAllocator
{
public:
    Ogre::Vector3 position;
    Ogre::Quaternion rotation;
    int type;
    float width;
    float bwidth;
    float bheight;
    int pillartype;
};

class ProceduralObject : public ZeroedMemoryAllocator
{
public:
    ProceduralObject() : loadingState(-1), name(""), road(0)
    {
    }

    int loadingState;
    std::string name;
    std::vector<ProceduralPoint> points;

    // runtime
    Road2* road;
};

class ProceduralManager : public ZeroedMemoryAllocator
{
protected:
    std::vector<ProceduralObject> pObjects;
    int objectcounter;

public:
    ProceduralManager();
    ~ProceduralManager();

    int addObject(ProceduralObject& po);

    int updateAllObjects();
    int updateObject(ProceduralObject& po);

    int deleteAllObjects();
    int deleteObject(ProceduralObject& po);

    std::vector<ProceduralObject>& getObjects();
};

} // namespace RoR
