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
#include "Ogre.h"
#include "SurveyMapEntity.h"

using namespace Ogre;

SurveyMapManager::SurveyMapManager(Vector3 worldSize) :
	  mWorldSize(worldSize)
	, mAlpha(1.0f)
	, mMapCenter(Vector3::ZERO)
	, mMapSize(Vector3::ZERO)
	, mMapZoom(0.0f)
{
	initialiseByAttributes(this);
	setVisibility(false);
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

void SurveyMapManager::updateRenderMetrics(RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
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

#endif // USE_MYGUI
