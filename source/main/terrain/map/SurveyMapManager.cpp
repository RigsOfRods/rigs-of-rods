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
#include "Beam.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "DustManager.h" // GfxScene
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SurveyMapEntity.h"
#include "SurveyMapTextureCreator.h"

using namespace Ogre;

SurveyMapManager::SurveyMapManager(Ogre::Vector2 terrain_size) :
      mAlpha(1.0f)
    , mMapCenter(terrain_size / 2)
    , mMapCenterThreshold(FSETTING("SurveyMapCenterThreshold", 5.0f))
    , mMapEntitiesVisible(true)
    , mMapMode(SURVEY_MAP_NONE)
    , mMapSize(terrain_size)
    , mMapTextureCreator(new SurveyMapTextureCreator(terrain_size))
    , mMapZoom(0.0f)
{
    initialiseByAttributes(this);
    setVisibility(false);

    mMapTexture->setImageTexture(mMapTextureCreator->getTextureName());
}

SurveyMapManager::~SurveyMapManager()
{
    for (auto entity : mMapEntities)
    {
        if (entity)
        {
            delete entity;
        }
    }
}

SurveyMapEntity* SurveyMapManager::createMapEntity(String type)
{
    auto entity = new SurveyMapEntity(this, mMapSize, type, mMapTexture);
    mMapEntities.insert(entity);
    return entity;
}

void SurveyMapManager::deleteMapEntity(SurveyMapEntity* entity)
{
    if (entity)
    {
        mMapEntities.erase(entity);
        delete entity;
    }
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

void SurveyMapManager::setVisibility(bool value)
{
    mMainWidget->setVisible(value);
}

void SurveyMapManager::windowResized()
{
    // TODO
}

void SurveyMapManager::updateRenderMetrics()
{
    if (RoR::App::GetOgreSubsystem()->GetRenderWindow())
        RoR::App::GetOgreSubsystem()->GetRenderWindow()->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

void SurveyMapManager::setMapZoom(Real zoomValue, bool permanent /*= true*/)
{
    zoomValue = Math::Clamp(zoomValue, 0.0f, 1.0f);

    if (mMapZoom == zoomValue)
        return;

    Real oldZoomValue = mMapZoom;

    mMapZoom = zoomValue;
    mMapTextureCreator->update();

    if (!permanent)
        mMapZoom = oldZoomValue;
}

void SurveyMapManager::setMapZoomRelative(Real zoomDelta, bool permanent /*= true*/)
{
    setMapZoom(mMapZoom + zoomDelta * std::max(0.1f, 1.0f - mMapZoom) / 100.0f, permanent);
}

void SurveyMapManager::setMapCenter(Vector2 position)
{
    if (mMapCenter == position)
        return;

    mMapCenter = position;
    mMapTextureCreator->update();
}

void SurveyMapManager::setMapCenter(Ogre::Vector2 position, float maxOffset)
{
    if (mMapCenter.distance(position) > std::abs(maxOffset))
        setMapCenter(position);
}

void SurveyMapManager::setMapCenter(Vector3 position)
{
    setMapCenter(Vector2(position.x, position.z));
}

void SurveyMapManager::setMapCenter(Ogre::Vector3 position, float maxOffset)
{
    setMapCenter(Vector2(position.x, position.z), maxOffset);
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

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_VIEW))
    {
        toggleMapView();
    }

    if (mMapMode == SURVEY_MAP_NONE)
        return;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_ICONS))
    {
        mMapEntitiesVisible = !mMapEntitiesVisible;
    }

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
        if (mMapZoom > 0.0f)
        {
            if (curr_truck)
            {
                auto& simbuf = curr_truck->GetGfxActor()->GetSimDataBuffer();
                setMapCenter(simbuf.simbuf_pos, mMapCenterThreshold * (1 - mMapZoom));
            }
            else
            {
                auto& simbuf = RoR::App::GetSimController()->GetGfxScene().GetSimDataBuffer();
                setMapCenter(simbuf.simbuf_character_pos, mMapCenterThreshold * (1 - mMapZoom));
            }
        }
        else
        {
            setMapCenter(mMapSize / 2);
        }
        break;

    case SURVEY_MAP_BIG:
        setMapCenter(mMapSize / 2);

        if (mMapZoom > 0.0f)
        {
            setMapZoom(0.0f, false);
        }

        // TODO: Is camera state owned by GfxScene or simulation? Let's access it as GfxScene for the time being ~ only_a_ptr, 05/2018
        if (RoR::App::GetSimController()->AreControlsLocked() ||
            RoR::App::GetSimController()->GetCameraBehavior() == RoR::CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
        {
            setWindowPosition(1, -1, 0.3f);
            mMapMode = SURVEY_MAP_SMALL;
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
}

void SurveyMapManager::toggleMapView()
{
    mMapMode = (mMapMode + 1) % SURVEY_MAP_END;

    if (mMapMode == SURVEY_MAP_BIG &&
        (RoR::App::GetSimController()->GetCameraBehavior() == RoR::CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM))
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
        mMapTextureCreator->update();
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

void SurveyMapManager::UpdateMapEntity(SurveyMapEntity* e, String caption, Vector3 pos, float rot, int state, bool visible)
{
    if (e)
    {
        e->setState(state);
        e->setRotation(rot);
        e->setCaption(caption);
        e->setPosition(pos.x, pos.z);
        e->setVisibility(visible && mMapEntitiesVisible && !mMapZoom);
    }
}
