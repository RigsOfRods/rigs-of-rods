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

#ifndef __MAP_CONTROL_H_
#define __MAP_CONTROL_H_

#include "RoRPrerequisites.h"

#include "mygui/BaseLayout.h"

class SurveyMapTextureCreator;

ATTRIBUTE_CLASS_LAYOUT(SurveyMapManager, "MapControl.layout");

class SurveyMapManager : public wraps::BaseLayout // TODO: public IManager ...
{
public:

	SurveyMapManager();

	SurveyMapEntity *createMapEntity(Ogre::String type);
	SurveyMapEntity *createNamedMapEntity(Ogre::String name, Ogre::String type);
	SurveyMapEntity *getMapEntityByName(Ogre::String name);
	void deleteMapEntity(SurveyMapEntity *entity);
	bool getMapEntitiesVisible() { return mMapEntitiesVisible; };

	void setAlpha(float alpha, bool permanent = true);
	float getAlpha() { return mMainWidget->getAlpha(); }

	void setVisibility(bool value);
	bool getVisibility();

	void setMapZoom(Ogre::Real zoomValue, bool update = true, bool permanent = true);
	void setMapZoomRelative(Ogre::Real zoomDelta, bool update = true, bool permanent = true);
	Ogre::Real getMapZoom() { return mMapZoom; }

	void setMapCenter(Ogre::Vector2 position, bool update = true);
	void setMapCenter(Ogre::Vector3 position, bool update = true);
	void setMapCenter(Ogre::Vector2 position, float maxOffset, bool update = true);
	void setMapCenter(Ogre::Vector3 position, float maxOffset, bool update = true);
	Ogre::Vector2 getMapCenter() { return mMapCenter; };

	void setMapTexture(Ogre::String name);

	Ogre::Vector3 getMapSize() { return mMapSize; };
	int getMapMode() { return mMapMode; };

	void windowResized();
	void setWindowPosition(int x, int y, float size);
	Ogre::Vector2 getWindowSize() { return Ogre::Vector2(realw, realh); };

	void toggleMapView();
	void toggleMapAlpha();

	void update(Ogre::Real dt);

	static Ogre::String getTypeByDriveable(int driveable);

	enum SurveyMapTypes { SURVEY_MAP_NONE, SURVEY_MAP_SMALL, SURVEY_MAP_BIG, SURVEY_MAP_END};

protected:

	void init();

	Ogre::Real mAlpha, mMapZoom;

	Ogre::Vector2 mMapCenter;
	Ogre::Vector3 mMapSize;

	ATTRIBUTE_FIELD_WIDGET_NAME(SurveyMapManager, mMapTexture, "mMapTexture");
	MyGUI::StaticImage* mMapTexture;

	SurveyMapTextureCreator* mMapTextureCreator;
	bool mMapTextureNeedsUpdate;

	std::map<Ogre::String, SurveyMapEntity *> mNamedEntities;
	std::set<SurveyMapEntity *> mMapEntities;
	bool mMapEntitiesVisible;

	void updateMapEntityPositions();
	void setMapEntitiesVisibility(bool visibility);

	int mMapMode;
	float mVelocity;

	float mMapCenterThreshold;

	int realw, realh;
	int rWinLeft, rWinTop;
	unsigned int rWinWidth, rWinHeight, rWinDepth;

	void updateRenderMetrics();
};

#endif // __MAP_CONTROL_H_

#endif // USE_MYGUI
