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



#include "SurveyMapManager.h"

#include "Application.h"
#include "BeamData.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "Character.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "Settings.h"
#include "SurveyMapEntity.h"
#include "SurveyMapTextureCreator.h"
#include "TerrainManager.h"

#include <Ogre.h>

using namespace Ogre;

SurveyMapManager::SurveyMapManager() :
    mAlpha(1.0f)
    , mMapCenter(Vector2::ZERO)
    , mMapCenterThreshold(5.0f)
    , mMapEntitiesVisible(false)
    , mMapMode(SURVEY_MAP_NONE)
    , mMapSize(Vector3::ZERO)
    , mMapTextureCreator(0)
    , mMapTextureNeedsUpdate(true)
    , mMapZoom(0.0f)
{
    initialiseByAttributes(this);
    setVisibility(false);

    gEnv->surveyMap = this;

    init();
}

SurveyMapManager::~SurveyMapManager()
{
    gEnv->surveyMap = nullptr;

    for (SurveyMapEntity* e : mMapEntities)
    {
        if (e)
            delete e;
    }
}

void SurveyMapManager::init()
{
    mMapSize = RoR::App::GetSimTerrain()->getMaxTerrainSize();
    mMapCenter = Vector2(mMapSize.x / 2.0f, mMapSize.z / 2.0f);

    mMapTextureCreator = new SurveyMapTextureCreator();
    mMapTextureCreator->update();
    setMapTexture(mMapTextureCreator->getTextureName());

    mMapCenterThreshold = FSETTING("SurveyMapCenterThreshold", 5.0f);
}

std::string SurveyMapManager::GetMinimapTextureName()
{
    return mMapTextureCreator->getTextureName();
}

SurveyMapEntity* SurveyMapManager::createMapEntity(String type)
{
    SurveyMapEntity* entity = new SurveyMapEntity(this, type, mMapTexture);
    mMapEntities.insert(entity);
    return entity;
}

SurveyMapEntity* SurveyMapManager::createNamedMapEntity(String name, String type)
{
    SurveyMapEntity* entity = createMapEntity(type);
    mNamedEntities[name] = entity;
    return entity;
}

void SurveyMapManager::deleteMapEntity(SurveyMapEntity* entity)
{
    mMapEntities.erase(entity);
}

SurveyMapEntity* SurveyMapManager::getMapEntityByName(String name)
{
    if (mNamedEntities.find(name) != mNamedEntities.end())
    {
        return mNamedEntities[name];
    }
    return nullptr;
}

bool SurveyMapManager::getVisibility()
{
    return mMainWidget->getVisible();
}

void SurveyMapManager::setAlpha(float alpha, bool permanent /*= true*/)
{
    mMainWidget->setAlpha(alpha);

    if (permanent)
        mAlpha = alpha;
}

void SurveyMapManager::setMapEntitiesVisibility(bool value)
{
    for (std::set<SurveyMapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
    {
        (*it)->setVisibility(value);
    }
}

void SurveyMapManager::setMapTexture(String name)
{
    mMapTexture->setImageTexture(name);
}

void SurveyMapManager::setVisibility(bool value)
{
    mMainWidget->setVisible(value);
}

void SurveyMapManager::windowResized()
{
    // TODO
}

void SurveyMapManager::updateMapEntityPositions()
{
    for (std::set<SurveyMapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
    {
        (*it)->update();
    }
}

void SurveyMapManager::updateRenderMetrics()
{
    if (RoR::App::GetOgreSubsystem()->GetRenderWindow())
        RoR::App::GetOgreSubsystem()->GetRenderWindow()->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

void SurveyMapManager::setMapZoom(Real zoomValue, bool update /*= true*/, bool permanent /*= true*/)
{
    if (mMapZoom == zoomValue)
        return;

    Real oldZoomValue = mMapZoom;

    mMapZoom = zoomValue;
    mMapZoom = std::max(0.0f, mMapZoom);
    mMapZoom = std::min(mMapZoom, 1.0f);

    if (update)
    {
        mMapTextureCreator->update();
        if (permanent)
            mMapTextureNeedsUpdate = true;
    }

    if (!permanent)
        mMapZoom = oldZoomValue;
}

void SurveyMapManager::setMapZoomRelative(Real zoomDelta, bool update /*= true*/, bool permanent /*= true*/)
{
    setMapZoom(mMapZoom + zoomDelta * std::max(0.1f, 1.0f - mMapZoom) / 100.0f, update, permanent);
}

void SurveyMapManager::setMapCenter(Vector2 position, bool update /*= true*/)
{
    if (mMapCenter == position)
        return;

    mMapCenter = position;

    if (update)
    {
        mMapTextureCreator->update();
        mMapTextureNeedsUpdate = true;
    }
}

void SurveyMapManager::setMapCenter(Ogre::Vector2 position, float maxOffset, bool update /*= true*/)
{
    if (mMapCenter.distance(position) > std::abs(maxOffset))
        setMapCenter(position, update);
}

void SurveyMapManager::setMapCenter(Vector3 position, bool update /*= true*/)
{
    setMapCenter(Vector2(position.x, position.z), update);
}

void SurveyMapManager::setMapCenter(Ogre::Vector3 position, float maxOffset, bool update /*= true*/)
{
    setMapCenter(Vector2(position.x, position.z), maxOffset, update);
}

void SurveyMapManager::setWindowPosition(int x, int y, float size)
{
    int realx = 0;
    int realy = 0;

    updateRenderMetrics();

    realw = realh = size * std::min(rWinWidth, rWinHeight);

    if (x == -1)
    {
        realx = 0;
    }
    else if (x == 0)
    {
        realx = (rWinWidth - realw) / 2;
    }
    else if (x == 1)
    {
        realx = rWinWidth - realw;
    }

    if (y == -1)
    {
        realy = 0;
    }
    else if (y == 0)
    {
        realy = (rWinHeight - realh) / 2;
    }
    else if (y == 1)
    {
        realy = rWinHeight - realh;
    }

    mMainWidget->setCoord(realx, realy, realw, realh);
}

Ogre::String SurveyMapManager::getTypeByDriveable(int driveable)
{
    switch (driveable)
    {
    case NOT_DRIVEABLE:
        return "load";
    case TRUCK:
        return "truck";
    case AIRPLANE:
        return "airplane";
    case BOAT:
        return "boat";
    case MACHINE:
        return "machine";
    default:
        return "unknown";
    }
}

void SurveyMapManager::Update(Ogre::Real dt, Actor* curr_truck)
{
    if (dt == 0)
        return;

    mVelocity = 0.0f;

    if (curr_truck)
    {
        mVelocity = curr_truck->ar_nodes[0].Velocity.length();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_VIEW))
    {
        toggleMapView();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_ICONS))
    {
        mMapEntitiesVisible = !mMapEntitiesVisible;
    }

    if (mMapMode == SURVEY_MAP_NONE)
        return;

    switch (mMapMode)
    {
    case SURVEY_MAP_SMALL:
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_ALPHA))
        {
            toggleMapAlpha();
        }
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_IN))
        {
            setMapZoomRelative(1.0f);
        }
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_OUT))
        {
            setMapZoomRelative(-1.0f);
        }
        if (getMapZoom() > 0.0f)
        {
            if (curr_truck)
            {
                setMapCenter(curr_truck->getPosition(), mMapCenterThreshold * (1 - mMapZoom));
            }
            else
            {
                setMapCenter(gEnv->player->getPosition(), mMapCenterThreshold * (1 - mMapZoom));
            }
        }
        else
        {
            setMapCenter(Vector3(mMapSize.x / 2.0f, 0.0f, mMapSize.z / 2.0f));
        }
        break;

    case SURVEY_MAP_BIG:
        setMapCenter(Vector3(mMapSize.x / 2.0f, 0.0f, mMapSize.z / 2.0f));

        if (getMapZoom() > 0.0f)
        {
            setMapZoom(0.0f, mMapTextureNeedsUpdate, false);
            mMapTextureNeedsUpdate = false;
        }

        if (curr_truck &&
            gEnv->cameraManager &&
            gEnv->cameraManager->hasActiveBehavior() &&
            !gEnv->cameraManager->gameControlsLocked())
        {
            if (mVelocity > 5.0f || gEnv->cameraManager->getCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
            {
                setWindowPosition(1, -1, 0.3f);
                //setAlpha(mAlpha);
                mMapMode = SURVEY_MAP_SMALL;
            }
            else
            {
                // Soft cross-fade into small map
                //setAlpha(1.0f / sqrt(std::max(1.0f, mVelocity - 1.0f)), false);
            }
        }
        else
        {
            if (getAlpha() != mAlpha)
            {
                setAlpha(mAlpha);
            }

            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_ALPHA))
            {
                toggleMapAlpha();
            }
        }
        break;

    default:
        break;
    }

    updateMapEntityPositions();
}

void SurveyMapManager::toggleMapView()
{
    mMapMode = (mMapMode + 1) % SURVEY_MAP_END;

    if (mMapMode == SURVEY_MAP_BIG && (mVelocity > 5.0f ||
    (gEnv->cameraManager &&
        gEnv->cameraManager->hasActiveBehavior() &&
        gEnv->cameraManager->getCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)))
    {
        mMapMode = (mMapMode + 1) % SURVEY_MAP_END;
    }

    if (mMapMode == SURVEY_MAP_NONE)
    {
        setVisibility(false);
    }
    else
    {
        if (mMapMode == SURVEY_MAP_SMALL)
        {
            setWindowPosition(1, -1, 0.3f);
        }
        else if (mMapMode == SURVEY_MAP_BIG)
        {
            setWindowPosition(0, 0, 0.98f);
        }
        setAlpha(mAlpha);
        setVisibility(true);
    }
}

void SurveyMapManager::toggleMapAlpha()
{
    if (getAlpha() > 0.61f)
    {
        setAlpha(0.6f);
    }
    else if (getAlpha() >= 0.31f && getAlpha() <= 0.61f)
    {
        setAlpha(0.3f);
    }
    else if (getAlpha() < 0.31f)
    {
        setAlpha(1.0f);
    }
}

void SurveyMapManager::UpdateVehicles(Actor** vehicles, int num_vehicles)
{
    for (int t = 0; t < num_vehicles; t++)
    {
        if (!vehicles[t])
            continue;
        SurveyMapEntity* e = getMapEntityByName("Truck" + TOSTRING(vehicles[t]->ar_instance_id));
        if (e)
        {
            e->setState(static_cast<int>(Actor::SimState::LOCAL_SIMULATED));
            e->setVisibility(true);
            e->setPosition(vehicles[t]->getPosition().x, vehicles[t]->getPosition().z);
            e->setRotation(Radian(vehicles[t]->getHeadingDirectionAngle()));
        }
    }
}


