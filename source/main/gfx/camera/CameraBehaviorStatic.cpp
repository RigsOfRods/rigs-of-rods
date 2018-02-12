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

#include "CameraBehaviorStatic.h"

#include <Ogre.h>

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Character.h"
#include "InputEngine.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;

CameraBehaviorStatic::CameraBehaviorStatic() :
    camPosition(Vector3::ZERO)
{
    updateTimer.reset();
}

bool intersectsTerrain(Vector3 a, Vector3 b)
{
    int steps = std::max(10.0f, a.distance(b));
    for (int i = 0; i < steps; i++)
    {
        Vector3 pos = a + (b - a) * (float)i / steps;
        float y = a.y + (b.y - a.y) * (float)i / steps;
        float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
        if (h > y)
        {
            return true;
        }
    }
    return false;
}

void CameraBehaviorStatic::update(const CameraManager::CameraContext& ctx)
{
    Vector3 lookAt = Vector3::ZERO;
    Vector3 velocity = Vector3::ZERO;
    Radian angle = Degree(90);
    float rotation = 0.0f;
    float speed = 0.0f;

    if (ctx.cct_player_actor)
    {
        lookAt = ctx.cct_player_actor->getPosition();
        rotation = ctx.cct_player_actor->getRotation();
        velocity = ctx.cct_player_actor->ar_nodes[0].Velocity * ctx.cct_sim_speed;
        angle = (lookAt - camPosition).angleBetween(velocity);
        speed = velocity.length();
        if (ctx.cct_player_actor->ar_replay_mode)
        {
            speed *= ctx.cct_player_actor->ar_replay_precision;
        }
    }
    else
    {
        lookAt = gEnv->player->getPosition();
        rotation = gEnv->player->getRotation().valueRadians();
    }

    bool forceUpdate = RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_RESET, 2.0f);
    forceUpdate = forceUpdate || (camPosition.distance(lookAt) > 200.0f && speed < 1.0f);

    if (forceUpdate || updateTimer.getMilliseconds() > 2000)
    {
        float distance = camPosition.distance(lookAt);
        bool terrainIntersection = intersectsTerrain(camPosition, lookAt + Vector3::UNIT_Y) || intersectsTerrain(camPosition, lookAt + velocity + Vector3::UNIT_Y);

        if (forceUpdate || terrainIntersection || distance > std::max(75.0f, speed * 3.5f) || (distance > 25.0f && angle < Degree(30)))
        {
            if (speed < 0.1f)
            {
                velocity = Vector3(cos(rotation), 0.0f, sin(rotation));
            }
            speed = std::max(5.0f, speed);
            camPosition = lookAt + velocity.normalisedCopy() * speed * 3.0f;
            Vector3 offset = (velocity.crossProduct(Vector3::UNIT_Y)).normalisedCopy() * speed;
            float r = (float)std::rand() / RAND_MAX;
            if (App::GetSimTerrain())
            {
                for (int i = 0; i < 100; i++)
                {
                    r = (float)std::rand() / RAND_MAX;
                    Vector3 pos = camPosition + offset * (0.5f - r) * 2.0f;
                    float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
                    pos.y = std::max(h, pos.y);
                    if (!intersectsTerrain(pos, lookAt + Vector3::UNIT_Y))
                    {
                        camPosition = pos;
                        break;
                    }
                }
            }
            camPosition += offset * (0.5f - r) * 2.0f;

            if (App::GetSimTerrain())
            {
                float h = App::GetSimTerrain()->GetHeightAt(camPosition.x, camPosition.z);

                camPosition.y = std::max(h, camPosition.y);
            }

            camPosition.y += 5.0f;

            updateTimer.reset();
        }
    }

    float camDist = camPosition.distance(lookAt);
    float fov = atan2(20.0f, camDist);

    gEnv->mainCamera->setPosition(camPosition);
    gEnv->mainCamera->lookAt(lookAt);
    gEnv->mainCamera->setFOVy(Radian(fov));
}

void CameraBehaviorStatic::activate(const CameraManager::CameraContext& ctx, bool reset /* = true */)
{
    fovPreviously = gEnv->mainCamera->getFOVy();
}

void CameraBehaviorStatic::deactivate(const CameraManager::CameraContext& ctx)
{
    gEnv->mainCamera->setFOVy(fovPreviously);
}
