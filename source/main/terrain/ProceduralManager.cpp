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

#include "ProceduralManager.h"

#include "Application.h"
#include "ProceduralRoad.h"

using namespace Ogre;
using namespace RoR;

#pragma region ProceduralObject

ProceduralPointPtr ProceduralObject::getPoint(int pos)
{
    if (pos >= 0 && pos < (int)points.size())
    {
        return points[pos];
    }
    else
    {
        return ProceduralPointPtr();
    }
}

void ProceduralObject::insertPoint(int pos, ProceduralPointPtr p)
{
    if (pos >= 0 && pos < (int)points.size())
    {
        points.insert(points.begin() + pos, p);
    }
}

void ProceduralObject::deletePoint(int pos)
{
    if (pos >= 0 && pos < (int)points.size())
    {
        points.erase(points.begin() + pos);
    }
}

#pragma endregion

#pragma region ProceduralManager

ProceduralManager::~ProceduralManager()
{
    this->removeAllObjects();
}

ProceduralObjectPtr ProceduralManager::getObject(int pos)
{
    if (pos >= 0 && pos < (int)pObjects.size())
    {
        return pObjects[pos];
    }
    else
    {
        return ProceduralObjectPtr();
    }
}

void ProceduralManager::removeAllObjects()
{
    for (ProceduralObjectPtr obj : pObjects)
    {
        this->deleteObject(obj);
    }
    pObjects.clear(); // delete (unreference) all objects.
}

void ProceduralManager::deleteObject(ProceduralObjectPtr po)
{
    if (po->road)
    {
        // loaded already, delete (unreference) old object
        po->road = ProceduralRoadPtr();
    }
}

void ProceduralManager::removeObject(ProceduralObjectPtr po)
{
    auto itor = pObjects.begin();
    while (itor != pObjects.end())
    {
        if (*itor == po)
        {
            this->deleteObject(*itor);
            itor = pObjects.erase(itor);
        }
        else
        {
            itor++;
        }
    }
}

void ProceduralManager::updateObject(ProceduralObjectPtr po)
{
    if (po->road)
        this->deleteObject(po);

    po->road = ProceduralRoadPtr::Bind(new ProceduralRoad());
    // In diagnostic mode, disable collisions (speeds up terrain loading)
    po->road->setCollisionEnabled(!App::diag_terrn_log_roads->getBool());

    for (ProceduralPointPtr& pp: po->points)
    {
        po->road->addBlock(pp->position, pp->rotation, pp->type, pp->width, pp->bwidth, pp->bheight, pp->pillartype);
    }
    po->road->finish();
}

void ProceduralManager::addObject(ProceduralObjectPtr po)
{
    updateObject(po);
    pObjects.push_back(po);
}

void ProceduralManager::logDiagnostics()
{
    Log("[RoR] Procedural road diagnostic.\n"
        "    types: 0=ROAD_AUTOMATIC, 1=ROAD_FLAT, 2=ROAD_LEFT, 3=ROAD_RIGHT, 4=ROAD_BOTH, 5=ROAD_BRIDGE, 6=ROAD_MONORAIL\n"
        "    pillartypes: 0=none, 1=road bridge, 2=monorail");
    for (int i=0; i< (int) pObjects.size(); ++i)
    {
        LogFormat("~~~~~~ ProceduralObject %d ~~~~~~", i);
        ProceduralObjectPtr& po=pObjects[i];
        for (int j = 0; j<(int)po->points.size(); ++j)
        {
            ProceduralPointPtr& pp = po->points[j];
            LogFormat("\t Point [%d] posXYZ %f %f %f, type %d, width %f, bwidth %f, bheight %f, pillartype %i",
                                j,   pp->position.x, pp->position.y, pp->position.z,
                                                   pp->type, pp->width, pp->bwidth, pp->bheight, pp->pillartype);
        }
    }
}

#pragma endregion
