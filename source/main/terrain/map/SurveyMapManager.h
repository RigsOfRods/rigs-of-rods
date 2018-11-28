/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#pragma once

#include "RoRPrerequisites.h"

#include "mygui/BaseLayout.h"

class SurveyMapTextureCreator;

ATTRIBUTE_CLASS_LAYOUT(SurveyMapManager, "MapControl.layout");

class SurveyMapManager : public wraps::BaseLayout
{
public:

    SurveyMapManager(Ogre::Vector2 terrain_size);
    ~SurveyMapManager();

    SurveyMapEntity* createMapEntity(Ogre::String type);
    void deleteMapEntity(SurveyMapEntity* entity);

    void setAlpha(float alpha, bool permanent = true);
    float getAlpha() { return mMainWidget->getAlpha(); }

    void setVisibility(bool value);
    bool getVisibility();

    void setMapZoom(Ogre::Real zoomValue, bool permanent = true);
    void setMapZoomRelative(Ogre::Real zoomDelta, bool permanent = true);
    Ogre::Real getMapZoom() { return mMapZoom; }

    void setMapCenter(Ogre::Vector2 position);
    void setMapCenter(Ogre::Vector3 position);
    void setMapCenter(Ogre::Vector2 position, float maxOffset);
    void setMapCenter(Ogre::Vector3 position, float maxOffset);
    Ogre::Vector2 getMapCenter() { return mMapCenter; };

    void windowResized();
    void setWindowPosition(int x, int y, float size);
    Ogre::Vector2 getWindowSize() { return Ogre::Vector2(realw, realh); };

    void toggleMapView();
    void toggleMapAlpha();

    void Update(Ogre::Real dt, Actor* curr_truck);

    static Ogre::String getTypeByDriveable(int driveable);

    enum SurveyMapTypes
    {
        SURVEY_MAP_NONE,
        SURVEY_MAP_SMALL,
        SURVEY_MAP_BIG,
        SURVEY_MAP_END
    };

    void UpdateMapEntity(SurveyMapEntity* e, Ogre::String caption, Ogre::Vector3 pos, float rot, int state, bool visible);

protected:

    Ogre::Real mAlpha, mMapZoom;

    Ogre::Vector2 mMapCenter;
    Ogre::Vector2 mMapSize;

    ATTRIBUTE_FIELD_WIDGET_NAME(SurveyMapManager, mMapTexture, "mMapTexture");

    MyGUI::StaticImage* mMapTexture;

    SurveyMapTextureCreator* mMapTextureCreator;

    std::set<SurveyMapEntity*> mMapEntities;
    bool mMapEntitiesVisible;

    int mMapMode;

    float mMapCenterThreshold;

    int realw, realh;
    int rWinLeft, rWinTop;
    unsigned int rWinWidth, rWinHeight, rWinDepth;

    void updateRenderMetrics();
};


