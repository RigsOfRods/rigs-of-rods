/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef USE_MYGUI

#include "SurveyMapManager.h"

#include "BeamData.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "InputEngine.h"
#include "Ogre.h"
#include "SurveyMapEntity.h"

using namespace Ogre;

SurveyMapManager::SurveyMapManager(Vector3 worldSize) :
	  mWorldSize(worldSize)
	, mAlpha(1.0f)
	, mMapCenter(Vector3::ZERO)
	, mMapMode(SURVEY_MAP_NONE)
	, mMapSize(Vector3::ZERO)
	, mMapZoom(0.0f)
	, mScale(1.0f)
	, mX(0)
	, mY(0)
{
	initialiseByAttributes(this);
	setVisibility(false);

	gEnv->surveyMap = this;
}

SurveyMapEntity *SurveyMapManager::createMapEntity(String type)
{
	SurveyMapEntity *entity = new SurveyMapEntity(this, type, mMapTexture);
	mMapEntities.insert(entity);
	return entity;
}

SurveyMapEntity *SurveyMapManager::createNamedMapEntity(String name, String type)
{
	SurveyMapEntity *entity = createMapEntity(type);
	mNamedEntities[name] = entity;
	return entity;
}

void SurveyMapManager::deleteMapEntity(SurveyMapEntity *entity)
{
	mMapEntities.erase(entity);
}

SurveyMapEntity *SurveyMapManager::getEntityByName(String name)
{
	if (mNamedEntities.find(name) != mNamedEntities.end())
	{
		return mNamedEntities[name];
	}
	return NULL;
}

bool SurveyMapManager::getVisibility()
{
	return mMainWidget->getVisible();
}

void SurveyMapManager::setAlpha(float value)
{
	mAlpha = value;
	mMainWidget->setAlpha(value);
}

void SurveyMapManager::setEntitiesVisibility(bool value)
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

void SurveyMapManager::updateEntityPositions()
{
	for (std::set<SurveyMapEntity *>::iterator it = mMapEntities.begin(); it != mMapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void SurveyMapManager::updateRenderMetrics()
{
	if (gEnv->renderWindow)
		gEnv->renderWindow->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

void SurveyMapManager::setMapZoom(Real zoomValue)
{
	mMapZoom = zoomValue;
	mMapZoom = std::max(0.0f, mMapZoom);
	mMapZoom = std::min(mMapZoom, 1.0f);
}

void SurveyMapManager::setMapZoomRelative(Real zoomDelta)
{
	setMapZoom(mMapZoom + zoomDelta * std::max(0.1f, 1.0f - mMapZoom) / 100.0f);
}

void SurveyMapManager::setMapCenter(Vector3 position)
{
	mMapCenter = position;
	mMapCenter.y = 0.0f;
}

void SurveyMapManager::setPosition(int x, int y, float size)
{
	int realx, realy, realw, realh;

	mScale = size;
	mX = x;
	mY = y;

	updateRenderMetrics();

	realw = realh = size * std::min(rWinWidth, rWinHeight);

	if (x == -1)
	{
		realx = 0;
	} else if (x == 0)
	{
		realx = (rWinWidth - realw) / 2;
	} else if (x == 1)
	{
		realx = rWinWidth - realw;
	}

	if (y == -1)
	{
		realy = 0;
	} else if (y == 0)
	{
		realy = (rWinHeight - realh) / 2;
	} else if (y == 1)
	{
		realy = rWinHeight - realh;
	}

	mMainWidget->setCoord(realx, realy, realw, realh);

	updateEntityPositions();
}

Ogre::String SurveyMapManager::getTypeByDriveable( int driveable )
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

void SurveyMapManager::update( Ogre::Real dt )
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	mVelocity = 0.0f;

	if (curr_truck)
	{
		mVelocity = curr_truck->nodes[0].Velocity.length();
	}

	if (curr_truck &&
		mMapMode == SURVEY_MAP_BIG &&
		gEnv->cameraManager &&
		gEnv->cameraManager->hasActiveBehavior() &&
		!gEnv->cameraManager->gameControlsLocked())
	{
		if (mVelocity > 7.5f || gEnv->cameraManager->getCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
		{
			setPosition(-1, 1, 0.3f);
			setAlpha(mAlpha);
			mMapMode = SURVEY_MAP_SMALL;
		} else
		{
			setAlpha(1.0f / sqrt(std::max(1.0f, mVelocity - 1.0f)));
		}
	}

	if (INPUTENGINE.getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_VIEW))
	{
		toggleMapView();
	}
	if (INPUTENGINE.getEventBoolValueBounce(EV_SURVEY_MAP_ALPHA))
	{
		toggleMapAlpha();
	}
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
	} else
	{
		if (mMapMode == SURVEY_MAP_SMALL)
		{
			setPosition(-1, 1, 0.3f);
		} else if (mMapMode == SURVEY_MAP_BIG)
		{
			setPosition(0, 0, 0.98f);
		}
		setAlpha(mAlpha);
		setVisibility(true);
	}
}

void SurveyMapManager::toggleMapAlpha()
{
	if (getAlpha() > 0.51f)
	{
		setAlpha(0.5f);
	}
	else if (getAlpha() >= 0.21f && getAlpha() <= 0.51f)
	{
		setAlpha(0.2f);
	}
	else if (getAlpha() < 0.21f)
	{
		setAlpha(1.0f);
	}
}

#endif // USE_MYGUI
