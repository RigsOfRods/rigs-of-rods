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
#include "ProceduralRoad.h"

namespace RoR {

/// @addtogroup Terrain
/// @{

struct ProceduralPoint
{
    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;
    int type = 0;
    float width = 0.f;
    float bwidth = 0.f;
    float bheight = 0.f;
    int pillartype = 0;
};

struct ProceduralObject
{
    std::vector<ProceduralPoint> points;
    ProceduralRoad* road = nullptr;
};

class ProceduralManager
{
public:
    ~ProceduralManager();

    int  addObject(ProceduralObject& po);

    int  deleteObject(ProceduralObject& po);

    void logDiagnostics();

private:
    int updateObject(ProceduralObject& po);

    std::vector<ProceduralObject> pObjects;
};

/// @} // addtogroup Terrain

} // namespace RoR
