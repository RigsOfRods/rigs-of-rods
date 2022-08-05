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

#include "ProceduralManager.h"

#include "Application.h"
#include "ProceduralRoad.h"

using namespace Ogre;
using namespace RoR;

ProceduralManager::~ProceduralManager()
{
    for (ProceduralObject& po : pObjects)
    {
        deleteObject(po);
    }
}

int ProceduralManager::deleteObject(ProceduralObject& po)
{
    if (po.road)
    {
        // loaded already, destroy old object
        delete po.road;
        po.road = 0;
    }
    return 0;
}

int ProceduralManager::updateObject(ProceduralObject& po)
{
    if (po.road)
        deleteObject(po);
    // create new road2 object
    po.road = new ProceduralRoad((int)pObjects.size());
    // In diagnostic mode, disable collisions (speeds up terrain loading)
    po.road->setCollisionEnabled(!App::diag_terrn_log_roads->getBool());

    std::vector<ProceduralPoint>::iterator it;
    for (it = po.points.begin(); it != po.points.end(); it++)
    {
        ProceduralPoint pp = *it;
        po.road->addBlock(pp.position, pp.rotation, pp.type, pp.width, pp.bwidth, pp.bheight, pp.pillartype);
    }
    po.road->finish();
    return 0;
}

int ProceduralManager::addObject(ProceduralObject& po)
{
    updateObject(po);
    pObjects.push_back(po);
    return 0;
}

void ProceduralManager::logDiagnostics()
{
    Log("[RoR] Procedural road diagnostic.\n"
        "    types: 0=ROAD_AUTOMATIC, 1=ROAD_FLAT, 2=ROAD_LEFT, 3=ROAD_RIGHT, 4=ROAD_BOTH, 5=ROAD_BRIDGE, 6=ROAD_MONORAIL\n"
        "    pillartypes: 0=none, 1=road bridge, 2=monorail");
    for (int i=0; i< (int) pObjects.size(); ++i)
    {
        LogFormat("~~~~~~ ProceduralObject %d ~~~~~~", i);
        ProceduralObject& po=pObjects[i];
        for (int j = 0; j<(int)po.points.size(); ++j)
        {
            ProceduralPoint& pt = po.points[j];
            LogFormat("\t Point [%d] posXYZ %f %f %f, type %d, width %f, bwidth %f, bheight %f, pillartype %i",
                                j,   pt.position.x, pt.position.y, pt.position.z,
                                                   pt.type, pt.width, pt.bwidth, pt.bheight, pt.pillartype);
        }
    }
}
