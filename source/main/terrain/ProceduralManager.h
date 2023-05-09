/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

// @author thomas, 17th of June 2008

#pragma once

#include "Application.h"
#include "ProceduralRoad.h"

namespace RoR {

/// @addtogroup Terrain
/// @{

struct ProceduralPoint: public RefCountingObject<ProceduralPoint>
{
    virtual ~ProceduralPoint() override {};

    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;
    RoadType type = RoadType::ROAD_AUTOMATIC;
    float width = 0.f;
    float bwidth = 0.f;
    float bheight = 0.f;
    int pillartype = 0;
};

struct ProceduralObject: public RefCountingObject<ProceduralObject>
{
    virtual ~ProceduralObject() override {};

    // Nice funcs for Angelscript
    void addPoint(ProceduralPointPtr p) { points.push_back(p); }
    ProceduralPointPtr getPoint(int pos);
    void insertPoint(int pos, ProceduralPointPtr p);
    void deletePoint(int pos);
    int getNumPoints() const { return (int)points.size(); }
    ProceduralRoadPtr getRoad() { return road; }
    std::string getName() { return name; }
    void setName(std::string const& new_name) { name = new_name; }

    std::string name;
    std::vector<ProceduralPointPtr> points;
    ProceduralRoadPtr road;
    int smoothing_num_splits = 0; // 0=off
};

class ProceduralManager: public RefCountingObject<ProceduralManager>
{
public:
    virtual ~ProceduralManager() override;

    /// Generates road mesh and adds to internal list
    void addObject(ProceduralObjectPtr po);

    /// Clears road mesh and removes from internal list
    void removeObject(ProceduralObjectPtr po);

    int getNumObjects() { return (int)pObjects.size(); }

    ProceduralObjectPtr getObject(int pos);

    void logDiagnostics();

    void removeAllObjects();

private:
    /// Rebuilds the road mesh
    void updateObject(ProceduralObjectPtr po);
    /// Deletes the road mesh
    void deleteObject(ProceduralObjectPtr po);

    std::vector<ProceduralObjectPtr> pObjects;
};

/// @} // addtogroup Terrain

} // namespace RoR
